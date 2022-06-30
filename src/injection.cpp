//
//  injection.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//
#include "injection.hpp"
#include "plist.hpp"

#define DYLIB_CURRENT_VER 0x10000
#define DYLIB_COMPATIBILITY_VERSION 0x10000
#define swap32(value) (((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) | ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24) )
#define INJECT_BEFORE_CODE_SIGNATURE 1

static int patch_binary(const std::filesystem::path & binaryPath,
                        const std::filesystem::path & dllPath,
                        bool isFramework,
                        bool remove_code_signature);

int inj_inject_framework_into_app(const std::filesystem::path &appPath, const std::filesystem::path &frameworkPath, bool remove_code_signature)
{
    int err = noErr;
    const std::string QUOTE = "\"";
    const std::string SPACE = " ";

    ORGLOG("Injecting framework ...");
    
    std::filesystem::path frameworksFolderPath = appPath;
    frameworksFolderPath.append("Contents/Frameworks");
    
    // Create destination folder.
    if (std::filesystem::exists(frameworksFolderPath) == false) {
        std::error_code error;
        if (!std::filesystem::create_directory(frameworksFolderPath, error)) {
            ORGLOG("Error creating Framework folder in the App: " << frameworksFolderPath << " Err: " << error.message());
            return ERR_General_Error;
        }
    }
        
    ORGLOG_V("Copying the framework into the App");
    std::string systemCmd = "/bin/cp -af " + QUOTE + (std::string)frameworkPath.c_str() + QUOTE + SPACE + QUOTE + frameworksFolderPath.c_str() + QUOTE;
    err = system((const char*)systemCmd.c_str());
    if (err) {
        ORGLOG("Failed copying the framework into the App.");
        return ERR_General_Error;
    }
    
    std::filesystem::path pathToInfoplist = appPath;
    pathToInfoplist.append("Contents/Info.plist");
    
    std::string binaryFileName = get_app_binary_file_name(pathToInfoplist);
    
    if (!binaryFileName.empty())  {
        std::filesystem::path pathToAppBinary = appPath / "Contents" / "MacOS";
        pathToAppBinary.append(binaryFileName);
        
        if (std::filesystem::exists(pathToAppBinary) == false) {
            ORGLOG("Could not find the binary file: " << pathToAppBinary);
            return ERR_General_Error;
        }
        
        err = patch_binary(pathToAppBinary, frameworkPath, true, remove_code_signature);
        if (err) {
            ORGLOG("Failed patching app binary.");
            return ERR_Injection_Failed;
        }
    } else {
        ORGLOG("Failed retrieving app binary file name.");
        return ERR_Injection_Failed;
    }
    return noErr;
}

void inj_inject_dylib(FILE* binaryFile, uint32_t top, const std::filesystem::path& dylibPath, bool remove_code_signature)
{
    
    // 1. Seek Slice start
    fseek(binaryFile, top, SEEK_SET); // go to slice start
    struct mach_header mach;
    
    // 2. Read the mac header to modify
    fread(&mach, sizeof(struct mach_header), 1, binaryFile); // read the mach header, it's at the beginning of the slice
    
    uint32_t load_dylib_size = (uint32_t)dylibPath.string().length() + sizeof(struct dylib_command);
    load_dylib_size += sizeof(long) - (load_dylib_size % sizeof(long)); // load commands like to be aligned by long
    
    uint32_t current_sizeofcmds = mach.sizeofcmds;
    if (remove_code_signature) {
        // the LC_CODE_SIGNATURE will be overwritten
        mach.sizeofcmds -= sizeof(struct linkedit_data_command);
        mach.sizeofcmds += load_dylib_size;
    } else {
        mach.ncmds += 1;
        mach.sizeofcmds += load_dylib_size;
    }

    // 3. Modify the mac header
    fseek(binaryFile, top, SEEK_SET); // go to slice start
    fwrite(&mach, sizeof(struct mach_header), 1, binaryFile); // write new mach0 header, advances to the start of load commands
    ORGLOG_V("Patching mach_header");
    
    // 4. GO TO THE END OF LAST COMMAND
    fseek(binaryFile, top, SEEK_SET); // go to slice start
    if (mach.magic == MH_MAGIC_64) {
        fseek(binaryFile, sizeof(struct mach_header_64)+current_sizeofcmds, SEEK_CUR); // go to the end of last command
    } else {
        fseek(binaryFile, sizeof(struct mach_header)+current_sizeofcmds, SEEK_CUR); // go to the end of last command
    }
    
    
#if INJECT_BEFORE_CODE_SIGNATURE
    struct linkedit_data_command lc_code_signature;
    if (!remove_code_signature) {
        // 5. Get LC_CODE_SIGNATURE to write it at the end later
        fseek(binaryFile, -sizeof(struct linkedit_data_command), SEEK_CUR);
        fread(&lc_code_signature, sizeof(struct linkedit_data_command), 1, binaryFile);
        if (lc_code_signature.cmd != LC_CODE_SIGNATURE) {
            ORGLOG("Did not find the LC_CODE_SIGNATURE command !");
            return;
        }
    }

    // 6. Write our load command on top of LC_CODE_SIGNATURE
    fseek(binaryFile, -sizeof(struct linkedit_data_command), SEEK_CUR);
#endif
    
    struct dylib_command dyld;
    fread(&dyld, sizeof(struct dylib_command), 1, binaryFile);
    
    ORGLOG_V("Attaching the library");
    
    dyld.cmd = LC_LOAD_DYLIB;
    dyld.cmdsize = load_dylib_size;
    dyld.dylib.compatibility_version = DYLIB_COMPATIBILITY_VERSION;
    dyld.dylib.current_version = DYLIB_CURRENT_VER;
    dyld.dylib.timestamp = 2;
    dyld.dylib.name.offset = sizeof(struct dylib_command);
    fseek(binaryFile, -sizeof(struct dylib_command), SEEK_CUR);
    fwrite(&dyld, sizeof(struct dylib_command), 1, binaryFile);
    
    size_t bytes = fwrite(dylibPath.string().data(), dylibPath.string().length(), 1, binaryFile); // write the dylib load string
    if (bytes==0) {
        ORGLOG_V("ERROR writing the loading path to the loading commands. In inject_dylib.");
    }
    
    // 7. Pad position. Maybe dylibPath is not exactly falling into size of long.
    if (dylibPath.string().length() % sizeof(long)) {
        fseek(binaryFile, sizeof(long) - (dylibPath.string().length() % sizeof(long)), SEEK_CUR);
    }
    
#if INJECT_BEFORE_CODE_SIGNATURE
    
    if (!remove_code_signature) {
        // 8. Write LC_CODE_SIGNATURE
        bytes = fwrite(&lc_code_signature, sizeof(struct linkedit_data_command), 1, binaryFile); // write the dylib load string
        if (bytes==0) {
            ORGLOG_V("ERROR writing LC_CODE_SIGNATURE to load commands.");
        }
    }

#endif
}

int patch_binary(const std::filesystem::path & binaryPath, const std::filesystem::path & dllPath, bool isFramework, bool remove_code_signature)
{
    ORGLOG("Patching app binary ...");
    
    std::filesystem::path dllName = dllPath.filename();
    std::filesystem::path dylibPath;
    
    if (isFramework) {
        std::filesystem::path frameworkName = dllPath.filename();
        std::filesystem::path frameworkNameNoExt = dllPath.filename().replace_extension("");
        dylibPath = std::filesystem::path("@executable_path/../Frameworks").append(frameworkName.string()).append(frameworkNameNoExt.string());
    } else {
        dylibPath = std::filesystem::path("@executable_path").append(dllName.string());
    }
    
    char buffer[4096];
    FILE *binaryFile = fopen(binaryPath.c_str(), "r+");
    if (binaryFile==NULL) {
        std::cout << "Error: Failed to open binary file: " << binaryPath << std::endl ;
        exit (ERR_General_Error);
    }
    fread(&buffer, sizeof(buffer), 1, binaryFile);
    
    struct fat_header* fh = (struct fat_header*) (buffer);
    
    if (fh->magic == FAT_CIGAM) {
        struct fat_arch* arch = (struct fat_arch*) &fh[1];
        ORGLOG_V("App has FAT binary");
        int i;
        for (i = 0; i < swap32(fh->nfat_arch); i++) {
            ORGLOG_V("Injecting to arch" << swap32(arch->cpusubtype));
            inj_inject_dylib(binaryFile, swap32(arch->offset), dylibPath, remove_code_signature);
            arch++;
        }
    } else {
        ORGLOG_V("App has thin binary");
        inj_inject_dylib(binaryFile, 0, dylibPath, remove_code_signature);
    }
    fclose(binaryFile);
    
    return 0;
}
