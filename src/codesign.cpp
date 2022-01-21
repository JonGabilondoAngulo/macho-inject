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

bool codesign_binary(const boost::filesystem::path & appPath,
                    const std::string& dllName,
                    const boost::filesystem::path& certificateFilePath,
                    const boost::filesystem::path& entitlementsFilePath)
{
    bool result = false;
    
    if (certificateFilePath.string().length() && appPath.string().length() && dllName.length()) {
        std::string systemCmd = (std::string)"/usr/bin/codesign -f -s '" + certificateFilePath.string() + "'" + silenceCmdOutput;
        
        // add entitlements to cmd
        if (entitlementsFilePath.string().length()) {
            systemCmd += (std::string)" --entitlements='" + entitlementsFilePath.string() + "' ";
        } else {
            systemCmd += " --preserve-metadata=entitlements";
        }
        
        // add dllpath to cmd
        systemCmd += " \"" + appPath.string() + "/" + dllName + "\"";
        
        int err = system(systemCmd.c_str());
        result = err==noErr;
        
        if (result) {
            ORGLOG_V("Success signing: " << (appPath / dllName));
        } else {
            ORGLOG_V("Error. Failed to sign: " << (appPath / dllName));
        }
    }
    
    return result;
}

bool codesign_at_path(const boost::filesystem::path & appPath,
                      const boost::filesystem::path&  certificateFilePath,
                      const boost::filesystem::path&  entitlementsFilePath)
{
    bool success = false;
    
    // Opening directory stream to app path
    DIR* dirFile = opendir(appPath.c_str());
    if (dirFile)
    {
        // Iterating over all files in app's path
        struct dirent* aFile;
        while ((aFile = readdir(dirFile)) != NULL)
        {
            // Ignoring current directory and parent directory names
            if (!strcmp(aFile->d_name, ".")) continue;
            if (!strcmp(aFile->d_name, "..")) continue;
            
            // Checking if file extension is 'dylib' OR 'framework'
            if (strstr(aFile->d_name, ".dylib") || strstr(aFile->d_name, ".framework") || strstr(aFile->d_name, ".appex")){
                
                // Found .dylib file, codesigning it
                codesign_binary(appPath, aFile->d_name, certificateFilePath, entitlementsFilePath);
            }
        }
        
        // Done iterating, closing directory stream
        closedir(dirFile);
        
        // Assuming all went right
        success = true;
    }
    
    return success;
}

int remove_codesign(const boost::filesystem::path& appPath)
{
    ORGLOG_V("Removing Code Signature");
    
    int err = 0;
    std::string pathToCodeSignature = appPath.string() + "/_CodeSignature";
    std::string systemCmd = (std::string)"rm -rf '" + (std::string)pathToCodeSignature + (std::string)"'";
    err = system((const char*)systemCmd.c_str());
    if(err)std::cout << "Error: Failed to remove codesignature file.\n";
    
    return err;
}

#pragma mark - Entitlements functions

int remove_entitlements(const boost::filesystem::path& appPath)
{
    ORGLOG_V("Removing Entitlements");
    
    int err = 0;
    std::string pathToEntitlements = appPath.string() + "/Entitlements.plist";
    std::string systemCmd = (std::string)"rm '" + (std::string)pathToEntitlements + (std::string)"'";
    err = system((const char*)systemCmd.c_str());
    if (err)std::cout << "Error: Failed to remove entitlements file.\n";
    
    return  err;
}

bool extract_entitlements_from_mobile_provision(const std::string& provisionFile,
                                                boost::filesystem::path& outNewEntitlementsFile,
                                                const boost::filesystem::path & tempDirPath)
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

bool extract_entitlements_from_app(const std::string& appPath, std::string& newEntitlementsFile, const std::string& tempDirPath)
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

int copy_provision_file(const boost::filesystem::path & appPath, const boost::filesystem::path& argProvision)
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

std::string resource_rules_file(bool forceResRules,
                                bool useOriginalResRules,
                                bool useGenericResRules,
                                const boost::filesystem::path & appPath,
                                const boost::filesystem::path & tempDirPath)
{
    std::string resRulesFile;
    bool resRulesSet = false;
    
    if (forceResRules || useOriginalResRules) {
        // apply original res rules
        // check if ResourceRules.plist is the App
        std::string pathToResourceRules = appPath.string() + "/" + "ResourceRules.plist";
        
        boost::filesystem::path boostPathToResourceRules = boost::filesystem::path(pathToResourceRules);
        if (boost::filesystem::exists(boostPathToResourceRules)) {
            ORGLOG_V("Codesign using original resource rules file :" << (std::string)pathToResourceRules);
            resRulesFile = pathToResourceRules;
            resRulesSet = true;
        }
    }
    if (resRulesSet == false) {
        if (forceResRules || useGenericResRules) {
            // Apply a generic res rules, create a local file with the template ResourceRulesTemplate
            boost::filesystem::path resourceRulesFile = tempDirPath;
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

bool verify_app_correctness(const boost::filesystem::path& appPath, const std::string& dllName)
{
    bool result = false;
    
    // Get the TeamIdentifier of the binaries
    std::string cmd = (std::string)"/usr/bin/codesign -dv \"" +  appPath.string() + "\" 2>&1 | grep TeamIdentifier=";
    std::string cmdResult1 = run_command(cmd);
    
    cmd = (std::string)"/usr/bin/codesign -dv \"" +  appPath.string() + "/" + dllName + "\" 2>&1 | grep TeamIdentifier=";
    std::string cmdResult2 = run_command(cmd);
    
    ORGLOG("Validating team identifiers");
    ORGLOG_V("App " << cmdResult1 << "DYLIB " <<  cmdResult2);
    
    result = (cmdResult2!="TeamIdentifier=not set" && cmdResult1.length() && cmdResult2.length() && cmdResult1==cmdResult2);
    
    return result;
}

