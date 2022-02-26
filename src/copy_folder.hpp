//
//  assisting_functions.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#ifndef copy_folder_hpp
#define copy_folder_hpp

#include <stdio.h>

bool copy_app(const std::filesystem::path &appPath,  const std::filesystem::path &destinationPath, std::filesystem::path &finalAppPath);
bool copy_bundle(const std::filesystem::path &bundlePath,  const std::filesystem::path &destinationPath);

#endif
