//
//  packaging.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 19/03/2017.
//

#include "packaging.hpp"


int unzip_ipa(const std::string argIpaPath,const std::string tempDirectory)
{
    ORGLOG("Unzipping the ipa.");
    
    int err = 0;
    std::string unzipPath = "unzip";
    
    std::string cmd = unzipPath + (std::string)" -o -q '" + (std::string)argIpaPath + (std::string)"' -d " + tempDirectory;
    
    ORGLOG_V("Unzipping the ipa with command : " + cmd);
    
    err = system((const char*)cmd.c_str());
    
    return err;
}


int repack( const boost::filesystem::path & unpackedAppPath,
           boost::filesystem::path & outRepackedIPAPath)
{
    ORGLOG("Packaging new ipa.");
    
    boost::filesystem::path zipPath;    
    zipPath = "zip";
    
    boost::filesystem::path repackedAppPath( boost::filesystem::temp_directory_path() / "machoRepackedIpa.ipa");
    
    // Go into the folder to zip with pushd and zip the contents into a file one level up.
    std::string systemCmd = "pushd " + (std::string)unpackedAppPath.c_str() + " > /dev/null; " + zipPath.c_str() + (std::string)" -qry " + repackedAppPath.c_str() + " *; popd > /dev/null";
    ORGLOG_V("Packaging Command:" + systemCmd);
    
    outRepackedIPAPath = repackedAppPath;
    return system((const char*)systemCmd.c_str());
}


int deploy_IPA(const boost::filesystem::path & inputIPAPath,
                                const boost::filesystem::path & originalIPAPath,
                                const std::string & newIPAName,
                                const boost::filesystem::path & newDestinationFolderPath,
                                bool wasInjected,
                                boost::filesystem::path & outNewIPAPath)
{
    boost::filesystem::path newAppPath;
    std::string newAppName;
    
    // Generating new ipa file name
    if (!newIPAName.empty()) {
        newAppName = newIPAName.c_str(); // user wants a new name
        ORGLOG_V((std::string)"Final ipa will have the name: " + newIPAName.c_str());
    } else {
        
        boost::filesystem::path originalIPAfilename = originalIPAPath.filename();
        newAppName = originalIPAfilename.string(); // In the simplest case this will be the final name
        
        if (wasInjected) {
            if (newAppName.find("+injected") == std::string::npos) {
                newAppName = originalIPAfilename.replace_extension("").string();
                newAppName += "+injected.ipa";
            }
        } else {
            // Add Codesigned suffix to name is it has not.
            if (newAppName.find("codesigned") == std::string::npos) {
                newAppName = originalIPAfilename.replace_extension("").string();
                newAppName += "-codesigned.ipa";
            }
        }
        ORGLOG_V((std::string)"Final ipa name: " + newAppName);
    }
    
    // Generating new ipa file path
    if (!newDestinationFolderPath.empty()) {
        newAppPath = newDestinationFolderPath / newAppName;
    } else {
        newAppPath = originalIPAPath;
        newAppPath.remove_filename().append(newAppName);
    }
    
    // Moving new ipa to its new path
    ORGLOG_V( (std::string)"Moving ipa to destination: " + newAppPath.string());
    std::string systemCmd = "mv -f " + (std::string)inputIPAPath.c_str() + (std::string)" \"" + newAppPath.string() + "\"";
    int err = system(systemCmd.c_str());
    
    outNewIPAPath = newAppPath;
    
    return err;
}

