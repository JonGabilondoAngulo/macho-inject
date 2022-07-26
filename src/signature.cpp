//
//  signature.cpp
//  macho_inject
//
//  Created by Jon Gabilondo on 28/02/2022.
//  Copyright © 2022 Organismo. All rights reserved.
//

#include <mach/machine.h>

#include "signature.hpp"
#include "plist.hpp"

#define swap32(value) (((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) | ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24) )

void sign_remove_signature_from_slice(FILE* binaryFile, uint32_t slice_offset)
{
    bool lcCodeSignatureExists = false;
    
    if (binaryFile==NULL) {
        ORGLOG("Mussing argument binary file.");
        exit (ERR_General_Error);
    }
    
    // Seek Slice start
    fseek(binaryFile, slice_offset, SEEK_SET); // go to slice start
    struct mach_header mach;
    
    // Read the mac header to modify
    fread(&mach, sizeof(struct mach_header), 1, binaryFile); // read the mach header, it's at the beginning of the slice
    uint32_t current_sizeofcmds = mach.sizeofcmds;

    // GO TO THE END OF LAST COMMAND
    fseek(binaryFile, slice_offset, SEEK_SET); // go to slice start
    if (mach.magic == MH_MAGIC_64) {
        fseek(binaryFile, sizeof(struct mach_header_64)+current_sizeofcmds, SEEK_CUR); // go to the end of last command
    } else {
        fseek(binaryFile, sizeof(struct mach_header)+current_sizeofcmds, SEEK_CUR); // go to the end of last command
    }
    
    // Is it LC_CODE_SIGNATURE
    struct linkedit_data_command lc_code_signature;
    fseek(binaryFile, -sizeof(struct linkedit_data_command), SEEK_CUR);
    fread(&lc_code_signature, sizeof(struct linkedit_data_command), 1, binaryFile);
    if (lc_code_signature.cmd == LC_CODE_SIGNATURE) {
        lcCodeSignatureExists = true;
    }
    
    if (lcCodeSignatureExists) {
        mach.sizeofcmds -= sizeof(struct linkedit_data_command);
        mach.ncmds -= 1;

        // Modify the mac header
        fseek(binaryFile, slice_offset, SEEK_SET); // go to slice start
        fwrite(&mach, sizeof(struct mach_header), 1, binaryFile); // write new mach0 header, advances to the start of load commands
        ORGLOG_V("Patching mach_header");
    }

}

std::string sign_get_cpu_type(cpu_type_t cpuType)
{
    switch (cpuType) {
        case CPU_TYPE_X86_64: return "CPU_TYPE_X86_64";
        case CPU_TYPE_ARM64: return "CPU_TYPE_ARM64";
        default: return "unknown";
    }
}

void sign_remove_signature_from_binary(FILE* binaryFile)
{
    char buffer[1024];

    if (binaryFile==NULL) {
        ORGLOG("Mussing argument binary file.");
        exit (ERR_General_Error);
    }
    fread(&buffer, sizeof(buffer), 1, binaryFile);
    
    struct fat_header* fh = (struct fat_header*) (buffer);
    
    if (fh->magic == FAT_CIGAM) {
        struct fat_arch* arch = (struct fat_arch*) &fh[1];
        ORGLOG_V("App has FAT binary");
        int i;
        for (i = 0; i < swap32(fh->nfat_arch); i++) {
            cpu_type_t cpuType = swap32(arch->cputype);
            
            ORGLOG_V("Removing signature from slice arch " << sign_get_cpu_type(cpuType));
            sign_remove_signature_from_slice(binaryFile, swap32(arch->offset));
            arch++;
        }
    } else {
        ORGLOG_V("App has a thin binary");
        ORGLOG_V("Removing signature from slice");
        sign_remove_signature_from_slice(binaryFile, 0);
    }
}

int sign_remove_code_signature_folder(const std::filesystem::path &appPath)
{
    ORGLOG_V("Removing Code Signature");
    
    int err = 0;
    std::string pathToCodeSignature = appPath.string() + "/Contents/_CodeSignature";
    std::string systemCmd = (std::string)"rm -rf '" + (std::string)pathToCodeSignature + (std::string)"'";
    err = system((const char*)systemCmd.c_str());
    if(err)
        std::cout << "Error: Failed to remove codesignature file.\n";
    
    return err;
}

#pragma mark Public

int sign_remove_signature_from_app(const std::filesystem::path &appPath)
{
    int err = sign_remove_code_signature_folder(appPath);
    if (err != noErr) {
        ORGLOG("Could not delete code signature folder.");
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
        
        FILE *binaryFile = fopen(pathToAppBinary.c_str(), "r+");
        sign_remove_signature_from_binary(binaryFile);
        fclose(binaryFile);

    } else {
        ORGLOG("Failed retrieving app binary file name.");
        return ERR_Injection_Failed;
    }
    return 0;
}

void sign_show_signature(FILE* binaryFile)
{
    
}
