//
//  version.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 19/03/2017.
//

#include "version.hpp"


void print_version()
{
    std::string osType = "OS X";
    
    int major       = 0;
    int minor       = 0;
    int revision    = 1;
    
    std::cout << "macho_inject version " << major << "." << minor << "." << revision << " (" << osType << ")" << " Copyright 2017 Organismo Software." << std::endl;
}
