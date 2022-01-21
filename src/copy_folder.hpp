//
//  assisting_functions.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#ifndef copy_folder_hpp
#define copy_folder_hpp

#include <stdio.h>

bool copy_app(const boost::filesystem::path &appPath,  const boost::filesystem::path &destinationPath, boost::filesystem::path &finalAppPath);
bool copy_bundle(const boost::filesystem::path &bundlePath,  const boost::filesystem::path &destinationPath);

#endif
