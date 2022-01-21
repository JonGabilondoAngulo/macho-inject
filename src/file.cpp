//
//  file.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 28/02/2017.
//

#include "file.hpp"

bool createFile(const boost::filesystem::path & filePath, const std::string & content, bool removeFirst)
{
    if (removeFirst && boost::filesystem::exists(filePath)) {
        boost::filesystem::remove(filePath);
    }
    
    boost::filesystem::ofstream fileStream(filePath);
    fileStream << content.data();
    
    return true;
}

