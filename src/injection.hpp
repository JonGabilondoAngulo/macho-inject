//
//  injection.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#ifndef injection_hpp
#define injection_hpp

#include <stdio.h>

int inj_inject_framework_into_app(const std::filesystem::path &appPath,
                                  const std::filesystem::path &frameworkPath,
                                  bool remove_code_signature);

void inj_inject_dylib(FILE* binaryFile, uint32_t top,
                      const std::filesystem::path& dylibPath,
                      bool remove_code_signature);

#endif /* injection_hpp */
