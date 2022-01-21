//
//  plist.cpp
//  macho-inject
//
//  Created by RJon Gabilondo on 3/14/17.
//  Copyright © 2017 Organismo. All rights reserved.
//

#include "plist.hpp"
#include <string.h>
#include <stdlib.h>

#pragma mark - Import / Export Functions

int plist_read_from_file(plist_t *plist, const char *filename)
{
    char *buffer = NULL;
    uint32_t length;
    
    if (!filename)
        return 0;
    
    buffer_read_from_file(filename, &buffer, &length);
    
    if (!buffer) {
        return 0;
    }
    
    if (memcmp(buffer, "bplist00", 8) == 0) {
        plist_from_bin(buffer, length, plist);
    } else {
        plist_from_xml(buffer, length, plist);
    }
    
    free(buffer);
    
    return 1;
}

int plist_write_to_file(plist_t plist, const char *filename, enum plist_format_t format)
{
    char *buffer = NULL;
    uint32_t length;
    
    if (!plist || !filename)
        return 0;
    
    if (format == PLIST_FORMAT_XML)
        plist_to_xml(plist, &buffer, &length);
    else if (format == PLIST_FORMAT_BINARY)
        plist_to_bin(plist, &buffer, &length);
    else
        return 0;
    
    buffer_write_to_file(filename, buffer, length);
    
    free(buffer);
    
    return 1;
}

void buffer_read_from_file(const char *filename, char **buffer, uint32_t *length)
{
    FILE *f;
    uint64_t size;
    
    f = fopen(filename, "rb");
    
    if(!f) {
        return;
    }
    
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);
    
    *buffer = (char*)malloc(sizeof(char)*size);
    fread(*buffer, sizeof(char), size, f);
    fclose(f);
    
    *length = (uint32_t)size;
}

void buffer_write_to_file(const char *filename, const char *buffer, uint64_t length)
{
    FILE *f;
    
    f = fopen(filename, "wb");
    if (f) {
        fwrite(buffer, sizeof(char), length, f);
        fclose(f);
    }
}


std::string get_app_binary_file_name(const boost::filesystem::path& infoPlistPath)
{
    plist_t pl = NULL;
    
    if(plist_read_from_file(&pl, infoPlistPath.c_str())) {
        plist_t bundleNode = plist_dict_get_item(pl, "CFBundleExecutable");
        if(bundleNode) {
            char * binaryName;
            plist_get_string_val(bundleNode, &binaryName);
            if(binaryName[0] != '\0') {
                return std::string(binaryName);
            }
        }
    }
    return "";
}
