//
//  injection.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#ifndef injection_hpp
#define injection_hpp

#include <stdio.h>

void inject_dylib(FILE* newFile, uint32_t top, const boost::filesystem::path& dylibPath);
int patch_binary(const boost::filesystem::path & binaryPath, const boost::filesystem::path & dllPath, bool isFramework);

#endif /* injection_hpp */
