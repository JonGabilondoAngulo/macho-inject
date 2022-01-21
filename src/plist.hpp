//
//  plist.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/14/17.
//  Copyright © 2017 Organismo. All rights reserved.
//

#ifndef plist_hpp
#define plist_hpp

#include <stdio.h>

enum plist_format_t {
    PLIST_FORMAT_XML,
    PLIST_FORMAT_BINARY
};

void buffer_write_to_file(const char *filename, const char *buffer, uint64_t length);
int plist_write_to_file(plist_t plist, const char *filename, enum plist_format_t format);
void buffer_read_from_file(const char *filename, char **buffer, uint32_t *length);
int plist_read_from_file(plist_t *plist, const char *filename);

std::string get_app_binary_file_name(const boost::filesystem::path& infoPlistPath);

#endif /* plist_hpp */
