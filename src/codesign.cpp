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
                        const std::filesystem::path &entitlementsFilePath,
                        const bool removeSignature)
{
    bool result = false;
    
    if (folderPath.string().empty() || fsObjectName.empty()) {
        return false;
    }
    
    // Codesign inner folders
    std::filesystem::path fsObjPath = folderPath;
    fsObjPath.append(fsObjectName);
    std::error_code errorCode;
    if (std::filesystem::is_directory(fsObjPath, errorCode) && !std::filesystem::is_symlink(fsObjPath, errorCode) ) {
        codesign_binaries_in_folder(fsObjPath, certificateFilePath, entitlementsFilePath, removeSignature);
    }

    
    // Codesign the object itself, could be a frameworkd, bundle, app
    std::string systemCmd = (std::string)"/usr/bin/codesign ";

    if (removeSignature) {
        systemCmd += " --remove-signature ";
    } else {
        if (certificateFilePath.string().empty()) {
            ORGLOG_V("Error. Ceritificate name not provided.");
            return false;
        }
        
        systemCmd += (std::string)" -f -s '" + certificateFilePath.string() + "'";
        
        // DO NOT add entitlements to frameworks !
        // https://developer.apple.com/documentation/xcode/using-the-latest-code-signature-format
        /*if (entitlementsFilePath.string().length()) {
            systemCmd += (std::string)" --entitlements='" + entitlementsFilePath.string() + "' ";
        } else {
            systemCmd += " --preserve-metadata=entitlements";
        }*/
        systemCmd += " --preserve-metadata ";
    }
        
    // add fsobj to cmd
    systemCmd += " \"" + folderPath.string() + "/" + fsObjectName + "\""  + silenceCmdOutput;
    
    int err = system(systemCmd.c_str());
    result = err==noErr;
    
    if (result) {
        if (removeSignature) {
            ORGLOG_V("Success removing signature: " << (folderPath / fsObjectName));
        } else {
            ORGLOG_V("Success signing: " << (folderPath / fsObjectName));
        }
    } else {
        if (removeSignature) {
            ORGLOG_V("Error. Failed removing signature " << (folderPath / fsObjectName));
        } else {
            ORGLOG_V("Error. Failed to sign: " << (folderPath / fsObjectName));
        }
    }

    return result;
}

bool codesign_binaries_in_folder(const std::filesystem::path &folderPath,
                                 const std::filesystem::path &certificateFilePath,
                                 const std::filesystem::path &entitlementsFilePath,
                                 const bool removeSignature)
{
    bool success = false;
    
    // Opening directory stream to app path
    DIR* dirFile = opendir(folderPath.c_str());
    if (dirFile)
    {
        // Iterating over all files in folder
        struct dirent* aFile;
        while ((aFile = readdir(dirFile)) != NULL)
        {
            // Ignoring current directory and parent directory names
            if (strcmp(aFile->d_name, ".") == 0) continue;
            if (strcmp(aFile->d_name, "..") == 0) continue;
                  
            std::filesystem::path extension = std::filesystem::path(aFile->d_name).extension();

            if (extension == ".dylib" ||
                extension == ".framework" ||
                extension == ".bundle" ||
                extension == ".app" ||
                extension == ".xpc" ||
                extension == ".node" ||
                extension == ".bin" ||
                strstr(extension.c_str(), "plugin") ||  // .*plugin
                extension == ".appex") {
                
                codesign_fs_object(folderPath, aFile->d_name, certificateFilePath, entitlementsFilePath, removeSignature);
            } else {
                std::filesystem::path fsObjPath = folderPath;
                fsObjPath.append(aFile->d_name);
                std::error_code errorCode;
                if (std::filesystem::is_directory(fsObjPath, errorCode) &&
                    !std::filesystem::is_symlink(fsObjPath, errorCode) &&
                    extension != ".lproj" &&
                    extension != ".pak" &&
                    extension != ".nib" 
                    )
                {
                    success = codesign_binaries_in_folder(fsObjPath, certificateFilePath, entitlementsFilePath, removeSignature);
                }
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

    // remove signature from all inner binaries
    const std::filesystem::path certificateFilePath;
    const std::filesystem::path entitlementsFilePath;
    codesign_binaries_in_folder(appPath, certificateFilePath, entitlementsFilePath, true);
    
    // remove signature from app
    std::string systemCmd = (std::string)"/usr/bin/codesign --remove-signature '" + appPath.string() + "'" + silenceCmdOutput;
    err = system((const char*)systemCmd.c_str());
    if (err) {
        std::cout << "Error: Failed to remove code signature from app.\n";
    }
        
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
    std::filesystem::path pathToProvision = appPath;
    // TODO: fix it for iOS
    pathToProvision.append("Contents");
    pathToProvision.append("embedded");
    pathToProvision.replace_extension(argProvision.extension());
    
    std::string systemCmd = (std::string)"/bin/cp -af \"" + argProvision.string() + "\" \"" + pathToProvision.u8string() + "\"";
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

