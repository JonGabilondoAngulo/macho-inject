//
//  file.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 28/02/2017.
//

#ifndef file_hpp
#define file_hpp

#include <stdio.h>

bool createFile(const std::filesystem::path & filePath, const std::string & content, bool removeFirst);
//bool isIpaFile(const std::filesystem::path & filePath);

#endif /* file_hpp */
