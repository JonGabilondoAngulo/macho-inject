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
int repack( const boost::filesystem::path & tempDirectoryPath,
           boost::filesystem::path & outRepackedIPAPath);
int deploy_IPA(const boost::filesystem::path & inputIPAPath,
                                const boost::filesystem::path & originalIPAPath,
                                const std::string & newIPAName,
                                const boost::filesystem::path & newDestinationFolderPath,
                                bool wasInjected,
                                boost::filesystem::path & outNewIPAPath);


#endif /* packaging_hpp */
