//
//  file.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 28/02/2017.
//

#include <fstream>
#include "file.hpp"

bool createFile(const std::filesystem::path & filePath, const std::string & content, bool removeFirst)
{
    if (removeFirst && std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);
    }
    
    std::ofstream fileStream(filePath);
    fileStream << content.data();
    
    return true;
}

