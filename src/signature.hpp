//
//  signature.hpp
//  macho_inject
//
//  Created by Jon Gabilondo on 28/02/2022.
//  Copyright © 2022 Organismo. All rights reserved.
//

#ifndef signature_hpp
#define signature_hpp

#include <stdio.h>

void sign_show_signature(FILE* binaryFile);

int sign_remove_signature_from_app(const std::filesystem::path &appPath);

#endif /* signature_hpp */
