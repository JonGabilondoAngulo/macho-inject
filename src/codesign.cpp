//
//  codesign.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#include "codesign.hpp"

const std::string sResourceRulesTemplate = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\
<plist version=\"1.0\">\n\
<dict>\n\
<key>rules</key>\n\
<dict>\n\
<key>.*</key>\n\
<true/>\n\
<key>Info.plist</key>\n\
<dict>\n\
<key>omit</key>\n\
<true/>\n\
<key>weight</key>\n\
<real>10</real>\n\
</dict>\n\
<key>ResourceRules.plist</key>\n\
<dict>\n\
<key>omit</key>\n\
<true/>\n\
<key>weight</key>\n\
<real>100</real>\n\
</dict>\n\
</dict>\n\
</dict>\n\
</plist>";

#pragma mark - Codesign functions

bool codesign_fs_object(const std::filesystem::path &folderPath,
                        const std::string &fsObjectName,
                        const std::filesystem::path &certificateFilePath,
                        const std::filesystem::path &entitlementsFilePath)
{
    bool result = false;
    
    if (!certificateFilePath.string().empty() && !folderPath.string().empty() && !fsObjectName.empty()) {
        std::string systemCmd = (std::string)"/usr/bin/codesign -f --deep -s '" + certificateFilePath.string() + "'" + silenceCmdOutput;
        
        // add entitlements to cmd
        if (entitlementsFilePath.string().length()) {
            systemCmd += (std::string)" --entitlements='" + entitlementsFilePath.string() + "' ";
        } else {
            systemCmd += " --preserve-metadata=entitlements";
        }
        
        // add dllpath to cmd
        systemCmd += " \"" + folderPath.string() + "/" + fsObjectName + "\"";
        
        int err = system(systemCmd.c_str());
        result = err==noErr;
        
        if (result) {
            ORGLOG_V("Success signing: " << (folderPath / fsObjectName));
        } else {
            ORGLOG_V("Error. Failed to sign: " << (folderPath / fsObjectName));
        }
    }
    
    return result;
}

bool codesign_binaries_in_folder(const std::filesystem::path &folderPath,
                                 const std::filesystem::path &certificateFilePath,
                                 const std::filesystem::path &entitlementsFilePath)
{
    bool success = false;
    
    // Opening directory stream to app path
    DIR* dirFile = opendir(folderPath.c_str());
    if (dirFile)
    {
        // Iterating over all files in app's path
        struct dirent* aFile;
        while ((aFile = readdir(dirFile)) != NULL)
        {
            // Ignoring current directory and parent directory names
            if (!strcmp(aFile->d_name, ".")) continue;
            if (!strcmp(aFile->d_name, "..")) continue;
            
            std::filesystem::path extension = std::filesystem::path(aFile->d_name).extension();
            
            // Checking if file extension
            if (extension == ".dylib" ||
                extension == ".framework" ||
                extension == ".bundle" ||
                extension == ".xpc" ||
                strstr(extension.c_str(), "plugin") ||  // .*plugin
                extension == ".appex") {
                
                codesign_fs_object(folderPath, aFile->d_name, certificateFilePath, entitlementsFilePath);
            }
        }
        
        // Done iterating, closing directory stream
        closedir(dirFile);
        
        // Assuming all went right
        success = true;
    }
    
    return success;
}

int codesign_remove_signature_from_app(const std::filesystem::path &appPath)
{
    ORGLOG_V("Removing Code Signature ...");
    
    int err = 0;

    std::string systemCmd = (std::string)"/usr/bin/codesign --deep --remove-signature '" + appPath.string() + "'" + silenceCmdOutput;
    err = system((const char*)systemCmd.c_str());
    if (err) {
        std::cout << "Error: Failed to remove code signature from app.\n";
    }
    
    /*
    std::string pathToCodeSignature = appPath.string() + "/Contents/_CodeSignature";
    std::string systemCmd = (std::string)"rm -rf '" + (std::string)pathToCodeSignature + (std::string)"'";
    err = system((const char*)systemCmd.c_str());
    if(err)
        std::cout << "Error: Failed to remove codesignature file.\n";
    */
    
    return err;
}

#pragma mark - Entitlements functions

int codesign_remove_entitlements(const std::filesystem::path &appPath)
{
    ORGLOG_V("Removing Entitlements");
    
    int err = 0;
    std::string pathToEntitlements = appPath.string() + "/Entitlements.plist";
    std::string systemCmd = (std::string)"rm '" + (std::string)pathToEntitlements + (std::string)"'";
    err = system((const char*)systemCmd.c_str());
    if (err)
        std::cout << "Error: Failed to remove entitlements file.\n";
    
    return  err;
}

bool codesign_extract_entitlements_from_mobile_provision(const std::string &provisionFile,
                                                std::filesystem::path & outNewEntitlementsFile,
                                                const std::filesystem::path &tempDirPath)
{
    bool success = false;
    
    std::string decodedProvision = tempDirPath.string() + "/../machoDecoded.mobileProvision"; // don't write it in our temp dir we need to zip all its contents
    
    ORGLOG_V("Getting entitlements from mobile provision: " << decodedProvision);
    
    std::string cmsCommand = (std::string)"security cms -D -i '" +  provisionFile + "' -o '" + decodedProvision + "'";
    int err = system((const char*)cmsCommand.c_str());
    if (err) {
        std::cout << "Error: Failed to decode mobile provision: " << provisionFile << " to: " <<  decodedProvision << std::endl;
    } else {
        // extract the entitlements
        
        std::string newEntitlements = tempDirPath.string() + "/../hpNewEntitlement.plist"; // don't write it in our temp dir we need to zip all its contents
        
        std::string plutilCommand = (std::string)"plutil -extract Entitlements xml1 -o " + newEntitlements + " " + decodedProvision;
        err = system((const char*)plutilCommand.c_str());
        if (err) {
            std::cout << "Error: Failed to extract entitlements from decoded mobile provision: " << decodedProvision << "to: " <<  newEntitlements << std::endl;
            success = false;
        } else {
            outNewEntitlementsFile = newEntitlements;
            success = true;
        }
    }
    return success;
}

bool codesign_extract_entitlements_from_app(const std::string &appPath, std::string &newEntitlementsFile, const std::string &tempDirPath)
{
    bool success = false;
    
    std::string entitlementsXml = tempDirPath + "/../hpNewEntitlement.xml";
    
    ORGLOG_V("Getting entitlements from app:" << appPath);
    
    std::string cmsCommand = (std::string)"/usr/bin/codesign -d --entitlements :- " + appPath + " > " + entitlementsXml;
    int err = system((const char*)cmsCommand.c_str());
    if (err) {
        std::cout << "Error: Failed to get entitlements from app file: " << appPath << std::endl;
    } else {
        newEntitlementsFile = entitlementsXml;
        success = true;
    }
    return success;
}

#pragma mark - Provision functions

int codesign_copy_provision_file(const std::filesystem::path &appPath, const std::filesystem::path &argProvision)
{
    ORGLOG_V("Adding provision file to app");
    
    int err = 0;
    std::string pathToProvision = appPath.string() + "/" + "embedded.mobileprovision";
    std::string systemCmd = (std::string)"/bin/cp -af \"" + argProvision.string() + "\" \"" + (std::string)pathToProvision + "\"";
    err = system((const char*)systemCmd.c_str());
    if(err)std::cout << "Error: Failed copying provision file. Profile:" << argProvision << " Destination:" << pathToProvision << std::endl;
    
    return err;
}

#pragma mark - Resource rules functions

std::string codesign_resource_rules_file(bool forceResRules,
                                bool useOriginalResRules,
                                bool useGenericResRules,
                                const std::filesystem::path &appPath,
                                const std::filesystem::path &tempDirPath)
{
    std::string resRulesFile;
    bool resRulesSet = false;
    
    if (forceResRules || useOriginalResRules) {
        // apply original res rules
        // check if ResourceRules.plist is the App
        std::string pathToResourceRules = appPath.string() + "/" + "ResourceRules.plist";
        
        std::filesystem::path boostPathToResourceRules = std::filesystem::path(pathToResourceRules);
        if (std::filesystem::exists(boostPathToResourceRules)) {
            ORGLOG_V("Codesign using original resource rules file :" << (std::string)pathToResourceRules);
            resRulesFile = pathToResourceRules;
            resRulesSet = true;
        }
    }
    if (resRulesSet == false) {
        if (forceResRules || useGenericResRules) {
            // Apply a generic res rules, create a local file with the template ResourceRulesTemplate
            std::filesystem::path resourceRulesFile = tempDirPath;
            resourceRulesFile.append("ResourceRules.plist");
            
            if (createFile(resourceRulesFile, sResourceRulesTemplate, true)) {
                ORGLOG_V("Codesign using general resource rules file :" << resourceRulesFile);
                resRulesFile = resourceRulesFile.string();
            }
        }
    }
    return resRulesFile;
}

#pragma mark - Validation functions

bool codesign_verify_app_correctness(const std::filesystem::path &appPath, const std::string &dllName)
{
    bool result = false;
    
    // Get the TeamIdentifier of the binaries
    std::string cmd = (std::string)"/usr/bin/codesign -dv \"" +  appPath.string() + "\" 2>&1 | grep TeamIdentifier=";
    std::string cmdResult1 = run_command(cmd);
    
    cmd = (std::string)"/usr/bin/codesign -dv \"" +  appPath.string() + "/" + dllName + "\" 2>&1 | grep TeamIdentifier=";
    std::string cmdResult2 = run_command(cmd);
    
    ORGLOG("Validating team identifiers");
    ORGLOG_V("App " << cmdResult1 << "DYLIB " <<  cmdResult2);
    
    result = (cmdResult2!="TeamIdentifier=not set" && !cmdResult1.empty() && !cmdResult2.empty() && cmdResult1==cmdResult2);
    
    return result;
}

