//
//  injection.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//
#include "injection.hpp"

#define DYLIB_CURRENT_VER 0x10000
#define DYLIB_COMPATIBILITY_VERSION 0x10000
#define swap32(value) (((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) | ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24) )
#define INJECT_BEFORE_CODE_SIGNATURE 0

void inject_dylib(FILE* newFile, uint32_t top, const boost::filesystem::path& dylibPath)
{
    
    // 1. Seek Slice start
    fseek(newFile, top, SEEK_SET); // go to slice start
    struct mach_header mach;
    
    // 2. Read the mac header to modify
    fread(&mach, sizeof(struct mach_header), 1, newFile); // read the mach header, it's at the beginning of the slice
    
    uint32_t dylib_size = (uint32_t)dylibPath.string().length() + sizeof(struct dylib_command);
    dylib_size += sizeof(long) - (dylib_size % sizeof(long)); // load commands like to be aligned by long
    
    mach.ncmds += 1;
    uint32_t current_sizeofcmds = mach.sizeofcmds;
    mach.sizeofcmds += dylib_size;
    
//    if (mach.magic == MH_MAGIC_64) {
//        uint32_t endOfComamndsPos = top + sizeof(struct mach_header) + sizeofcmds;
//        if (endOfComamndsPos % sizeof(long)) {
//            sizeofcmds += endOfComamndsPos % sizeof(long);
//        }
//    }
    
    // 3. Modify the mac header
    //fseek(newFile, -sizeof(struct mach_header), SEEK_CUR); // back to slice top
    fseek(newFile, top, SEEK_SET); // go to slice start
    fwrite(&mach, sizeof(struct mach_header), 1, newFile); // write new mach0 header, advances to the start of load commands
    ORGLOG_V("Patching mach_header");
    
    // 4. GO TO THE END OF LAST COMMAND
    fseek(newFile, top, SEEK_SET); // go to slice start
    //fseek(newFile, sizeof(struct mach_header)+sizeofcmds, SEEK_CUR); // go to the end of last command
    if (mach.magic == MH_MAGIC_64) {
        fseek(newFile, sizeof(struct mach_header_64)+current_sizeofcmds, SEEK_CUR); // go to the end of last command
    } else {
        fseek(newFile, sizeof(struct mach_header)+current_sizeofcmds, SEEK_CUR); // go to the end of last command
    }
    
    
#if INJECT_BEFORE_CODE_SIGNATURE
    // 5. Get LC_CODE_SIGNATURE to write it at the end later
    struct linkedit_data_command lc_code_signature;
    fseek(newFile, -sizeof(struct linkedit_data_command), SEEK_CUR);
    fread(&lc_code_signature, sizeof(struct linkedit_data_command), 1, newFile);
    if (lc_code_signature.cmd != LC_CODE_SIGNATURE) {
        ORGLOG("Did not find the LC_CODE_SIGNATURE command !");
        return;
    }

    // 6. Write our load command on top of LC_CODE_SIGNATURE
    fseek(newFile, -sizeof(struct linkedit_data_command), SEEK_CUR);
#endif
    
    struct dylib_command dyld;
    fread(&dyld, sizeof(struct dylib_command), 1, newFile);
    
    ORGLOG_V("Attaching the library");
    
    dyld.cmd = LC_LOAD_DYLIB;
    dyld.cmdsize = dylib_size;
    dyld.dylib.compatibility_version = DYLIB_COMPATIBILITY_VERSION;
    dyld.dylib.current_version = DYLIB_CURRENT_VER;
    dyld.dylib.timestamp = 2;
    dyld.dylib.name.offset = sizeof(struct dylib_command);
    fseek(newFile, -sizeof(struct dylib_command), SEEK_CUR);
    fwrite(&dyld, sizeof(struct dylib_command), 1, newFile);
    
    size_t bytes = fwrite(dylibPath.string().data(), dylibPath.string().length(), 1, newFile); // write the dylib load string
    if (bytes==0) {
        ORGLOG_V("ERROR writing the loading path to the loading commands. In inject_dylib.");
    }
    
    // 7. Pad position. Maybe dylibPath is not exactly falling into size of long.
    if (dylibPath.string().length() % sizeof(long)) {
        fseek(newFile, sizeof(long) - (dylibPath.string().length() % sizeof(long)), SEEK_CUR);
    }
    
#if INJECT_BEFORE_CODE_SIGNATURE
    // 8. Write LC_CODE_SIGNATURE
    bytes = fwrite(&lc_code_signature, sizeof(struct linkedit_data_command), 1, newFile); // write the dylib load string
    if (bytes==0) {
        ORGLOG_V("ERROR writing LC_CODE_SIGNATURE to load commands.");
    }
#endif
}

int patch_binary(const boost::filesystem::path & binaryPath, const boost::filesystem::path & dllPath, bool isFramework)
{
    ORGLOG("Patching app binary");
    
    boost::filesystem::path dllName = dllPath.filename();
    boost::filesystem::path dylibPath;
    
    if (isFramework) {
        boost::filesystem::path frameworkName = dllPath.filename();
        boost::filesystem::path frameworkNameNoExt = dllPath.filename().replace_extension("");
        dylibPath = boost::filesystem::path("@rpath").append(frameworkName.string()).append(frameworkNameNoExt.string());
    } else {
        dylibPath = boost::filesystem::path("@executable_path").append(dllName.string());
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
            inject_dylib(binaryFile, swap32(arch->offset), dylibPath);
            arch++;
        }
    }
    else {
        ORGLOG_V("App has thin binary");
        inject_dylib(binaryFile, 0, dylibPath);
    }
    fclose(binaryFile);
    
    return 0;
}
