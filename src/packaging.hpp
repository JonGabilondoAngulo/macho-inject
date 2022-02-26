//
//  packaging.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 19/03/2017.
//

#ifndef packaging_hpp
#define packaging_hpp

#include <stdio.h>

int unzip_ipa(const std::string argIpaPath,const std::string tempDirectory);
int repack( const std::filesystem::path & tempDirectoryPath,
           std::filesystem::path & outRepackedIPAPath);
int deploy_IPA(const std::filesystem::path & inputIPAPath,
                                const std::filesystem::path & originalIPAPath,
                                const std::string & newIPAName,
                                const std::filesystem::path & newDestinationFolderPath,
                                bool wasInjected,
                                std::filesystem::path & outNewIPAPath);


#endif /* packaging_hpp */
