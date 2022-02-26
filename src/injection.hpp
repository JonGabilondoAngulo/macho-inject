//
//  injection.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#ifndef injection_hpp
#define injection_hpp

#include <stdio.h>

int inj_inject_framework_into_app(const boost::filesystem::path &appPath,
                                  const boost::filesystem::path &frameworkPath,
                                  bool remove_code_signature);

void inj_inject_dylib(FILE* binaryFile, uint32_t top,
                      const boost::filesystem::path& dylibPath,
                      bool remove_code_signature);

#endif /* injection_hpp */
