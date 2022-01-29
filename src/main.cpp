//
//  main.mm
//  macho_inject
//
//  Created by Jon Gabilondo on 9/29/14.
//

#include <stdio.h>
#include "codesign.hpp"
#include "injection.hpp"
#include "packaging.hpp"
#include "plist.hpp"
#include "copy_folder.hpp"

bool doInjectFramework      = false;
bool doCodesign             = false;
bool copyEntitlements       = false;
bool useResourceRules       = false;
bool removeEntitlements     = false;
bool removeCodeSignature    = false;
bool createNewUrlScheme     = true;
bool useOriginalResRules    = false;
bool useGenericResRules     = false;
bool forceResRules          = false;
bool isIpaFile              = false;
bool isAppFile              = false;

boost::filesystem::path argTargetFilePath;
boost::filesystem::path argFrameworkPath;
boost::filesystem::path argCertificatePath;
boost::filesystem::path argProvisionPath;
boost::filesystem::path argEntitlementsPath;
boost::filesystem::path argResourceRulesPath;
boost::filesystem::path argDestinationPath;
boost::filesystem::path argNewName;
boost::filesystem::path repackedAppPath;
boost::filesystem::path workingAppPath;
boost::filesystem::path tempDirectoryPath;
boost::filesystem::path tempDirectoryIPA;;

static const std::string SPACE = " ";
static const std::string QUOTE = "\"";

int parse_options(int argc, const char * argv[])
{
    int err = 0;

    boost::program_options::options_description desc("Allowed options", 200); // 200 cols
    boost::program_options::variables_map vm;
    cmd_opt_add_descriptions(desc);
    err = cmd_opt_parse_options(argc, argv, desc, vm);
    if (err != noErr) {
        ORGLOG("Error parsing options.");
        return err;
    }
    
    if (vm.count("help")) {
        std::cout << "Usage: macho_inject -f framework -s certificate -p provision [-v] [-h] [-V]  target_file (.app|.ipa) \n";
        std::cout << desc << "\n";
    }
    if (vm.count("version")) {
        print_version();
        return 0;
    }
    if (vm.count("verbose")) {
        verbose = true;
        silenceCmdOutput = "";
    }
    if (vm.count("input-file")) {
        argTargetFilePath = vm["input-file"].as<std::string>();
    }
    if (vm.count("inject-framework")) {
        argFrameworkPath = vm["inject-framework"].as<std::string>();
        doInjectFramework = true;
    }
    if (vm.count("sign")) {
        argCertificatePath = vm["sign"].as<std::string>();
        doCodesign = true;
    }
    if (vm.count("provision")) {
        argProvisionPath = vm["provision"].as<std::string>();
    }
    if (vm.count("entitlements")) {
        argEntitlementsPath = vm["entitlements"].as<std::string>();
        copyEntitlements = true;
    }
    if (vm.count("resource-rules")) {
        argResourceRulesPath = vm["resource-rules"].as<std::string>();
        useResourceRules = true;
    }
    if (vm.count("output-dir")) {
        argDestinationPath = vm["output-dir"].as<std::string>();
    }
    if (vm.count("output-name")) {
        argNewName = vm["output-name"].as<std::string>();
    }
    if (vm.count("no-url-scheme")) {
        createNewUrlScheme = false;
    }
    if (vm.count("original-res-rules")) {
        useOriginalResRules = true;
    }
    if (vm.count("generic-res-rules")) {
        useGenericResRules = true;
    }
    if (vm.count("force-res-rules")) {
        forceResRules = true;
    }
    if (vm.count("remove-entitlements")) {
        removeEntitlements = true;
    }
    if (vm.count("remove-code-signature")) {
        removeCodeSignature = true;
    }
    return err;
}

int check_arguments()
{
    if (!argTargetFilePath.empty()) {
        if (!boost::filesystem::exists(argTargetFilePath)) {
            ORGLOG("Error: target file not found at: " << argTargetFilePath);
            return (ERR_Bad_Argument);
        }
        boost::filesystem::path extension = argTargetFilePath.extension();
        if (extension != ".ipa" && extension != ".app") {
            ORGLOG("Error: target file must be .app or .ipa. File: " << argTargetFilePath);
            return (ERR_Bad_Argument);
        }
        isIpaFile = (extension == ".ipa");
        isAppFile = (extension == ".app");

    } else {
        ORGLOG("Error: Missing target file.");
        return (ERR_Bad_Argument);
    }
    
    if (doInjectFramework) {
        if (!boost::filesystem::exists(argFrameworkPath)) {
            ORGLOG("Framework file not found at: " << argFrameworkPath);
            return (ERR_Bad_Argument);
        }
    }
    
    if (!argProvisionPath.empty()) {
        if (!boost::filesystem::exists(argProvisionPath)) {
            ORGLOG("Provision Profile not found at: " << argProvisionPath);
            return (ERR_Bad_Argument);
        }
    }
    
    if (!argDestinationPath.empty() && !boost::filesystem::exists(argDestinationPath)) {
        ORGLOG("Destination folder does not exist: " << argDestinationPath);
        return (ERR_Bad_Argument);
    }
    return noErr;
}

int codesign_app()
{
    int err = noErr;

    ORGLOG("Codesigning the app ...");
    
    if (!argProvisionPath.empty()) {
        err = copy_provision_file(workingAppPath, argProvisionPath);
        if (err) {
            return ERR_General_Error;
        }
    }
    
    if (removeEntitlements) {
        err = remove_entitlements(workingAppPath);
    }
    
    if (removeCodeSignature) {
        err = remove_codesign(workingAppPath);
    }
    
    std::string systemCmd = (std::string)"/usr/bin/codesign --deep --force -s '" + (std::string)argCertificatePath.c_str() + (std::string)"' ";
    
    // Important entitlements info:
    // no entitlements provided in arguments, extract them from mobile provision
    // the provision file could be given in argument, if not extract it from the embeded.mobileprovision
    boost::filesystem::path entitlementsFilePath;
    
    if (copyEntitlements) {
        entitlementsFilePath = argEntitlementsPath;
        
        ORGLOG_V("Codesign using entitlements. " << entitlementsFilePath);
        systemCmd += (std::string)" --entitlements='" + entitlementsFilePath.string() + "'";
    } else {
        ORGLOG_V("Codesign preserving entitlements.");
        systemCmd += " --preserve-metadata=entitlements";
    }
                
    std::string resRulesFile;
    if (useResourceRules) {
        resRulesFile = argResourceRulesPath.c_str();
    } else {
        resRulesFile = resource_rules_file(forceResRules, useOriginalResRules, useGenericResRules, workingAppPath, tempDirectoryPath);
    }
    
    if (resRulesFile.length()) {
        ORGLOG_V("Codesign using resource rules file :" << (std::string)resRulesFile);
        systemCmd += (std::string)" --resource-rules='" + resRulesFile + "'";
    }
    
    systemCmd += " \"" + workingAppPath.string() + "\"";
    
    // Codesign dylibs
    if (!codesign_at_path(workingAppPath, argCertificatePath, entitlementsFilePath)) {
        err = ERR_Dylib_Codesign_Failed;
        ORGLOG("Codesigning one or more dylibs failed");
    }
    
    // Codesigning extensions under plugins folder
    boost::filesystem::path extensionsFolderPath = workingAppPath;
    extensionsFolderPath.append("Contents/PlugIns");
    if (codesign_at_path(extensionsFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("Plugins codesign sucessful!");
    }
    
    // Codesigning frameworks
    boost::filesystem::path frameworksFolderPath = workingAppPath;
    frameworksFolderPath.append("Contents/Frameworks");
    if (codesign_at_path(frameworksFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("Frameworks codesign sucessful!");
    }
    
    if (!silenceCmdOutput.empty()) {
        systemCmd += SPACE + silenceCmdOutput;
    }

    // sign the app
    err = system((const char*)systemCmd.c_str());
    if (err) {
        ORGLOG("Codesigning app failed");
        return  ERR_App_Codesign_Failed;
    }

    return err;
}

int prepare_folders()
{
    int err = 0;
    
    // Create temp folder path
    tempDirectoryPath = boost::filesystem::temp_directory_path() / "macho_inject";

    // Remove old temp if exist
    if (boost::filesystem::exists(tempDirectoryPath)) {
        std::string systemCmd = (std::string)"rm -rf " + tempDirectoryPath.string();
        err = system(systemCmd.c_str());
        if (err != noErr) {
            ORGLOG("Error: Failed to delete temp folder. " << tempDirectoryPath);
            return (ERR_General_Error);
        }
    }
    
    // Remove old ipa
    tempDirectoryIPA = tempDirectoryPath;
    tempDirectoryIPA.replace_extension("ipa");
    if (boost::filesystem::exists(tempDirectoryIPA)) {
        std::string systemCmd = (std::string)"rm -rf " + tempDirectoryIPA.string();
        err = system(systemCmd.c_str());
        if (err != noErr) {
            ORGLOG("Error: Failed to delete folder. " << tempDirectoryIPA.string());
            return (ERR_General_Error);
        }
    }
    
    if (isIpaFile) {
        // Unzip ipa file
        std::string tempDirectoryStd = tempDirectoryPath.string();
        ORGLOG_V("Unziping IPA into: " + tempDirectoryStd);
        if (unzip_ipa(argTargetFilePath.c_str(), tempDirectoryStd)) {
            ORGLOG("Error: Failed to unzip ipa.\n");
            return (ERR_General_Error);
        }
        
        // Create app path and app name
        std::string systemCmd = (std::string)"ls -d " + tempDirectoryPath.string() + "/Payload/*.app";
        std::string cmdResult = run_command(systemCmd);
        boost::filesystem::path appName = boost::filesystem::path(cmdResult).filename();
        boost::filesystem::path appNameWithoutExtension = boost::filesystem::path(cmdResult).filename().replace_extension("");
        workingAppPath = tempDirectoryPath;
        workingAppPath.append("Payload").append(appName.string());
    } else if (isAppFile) {
        if (!copy_bundle(argTargetFilePath, tempDirectoryPath)) {
            ORGLOG("Error: Failed copying target file to temp folder.: " << workingAppPath);
            return (ERR_General_Error);
        }
        workingAppPath = tempDirectoryPath / argTargetFilePath.filename();
    } else {
        ORGLOG("Error: file is not ipa/app.");
        return (ERR_Bad_Argument);
    }
    ORGLOG_V("Path to app in temp folder: " << workingAppPath);
    
    return noErr;
}

int deploy_app_in_destination()
{
    int err = noErr;
    
    if (isIpaFile) {
        err = repack(tempDirectoryPath, repackedAppPath);
        if (err) {
            ORGLOG("Error. Failed to zip ipa.");
        } else {
            boost::filesystem::path newAppPath;
            int err = deploy_IPA(repackedAppPath, argTargetFilePath, argNewName.c_str(), argDestinationPath, doInjectFramework, newAppPath);
            if (err) {
                ORGLOG("Failed to move the new ipa to destination: " << newAppPath);
            } else {
                ORGLOG("\nSUCCESS!");
                ORGLOG("\nNew IPA path: " << newAppPath);
            }
         }
    } else if (isAppFile) {
        if (!argDestinationPath.empty()) {
            boost::filesystem::path finalAppPath;
            if (copy_app(workingAppPath, argDestinationPath, finalAppPath)) {
                ORGLOG("New App path: " << finalAppPath);
                ORGLOG("\nSUCCESS!");
            } else {
                err = ERR_General_Error;
                ORGLOG("Error copying app to destination: " << argDestinationPath);
            }
        } else {
            ORGLOG("New App path: " << workingAppPath);
            ORGLOG("\nSUCCESS!");
        }
    } else {
        ORGLOG("Error: file is not ipa/app.");
        err = ERR_Bad_Argument;
    }
    return err;
}

int main(int argc, const char * argv[])
{
    int err                     = 0;

    err = parse_options(argc, argv);
    if (err != noErr)
        exit(err);
    
    err = check_arguments();
    if (err != noErr)
        exit(err);

    err = prepare_folders();
    if (err != noErr)
        exit(err);


    if (doInjectFramework) {
        err = inj_inject_framework_into_app(workingAppPath, argFrameworkPath);
        if (err != noErr)
            goto CLEAN_EXIT;
    }
    
    if (doCodesign) {
        err = codesign_app();
        if (err != noErr)
            exit(err);
    }
    
    err = deploy_app_in_destination();
        
    
CLEAN_EXIT:
    
    if (boost::filesystem::exists(tempDirectoryPath)) {
        if (!boost::filesystem::remove_all(tempDirectoryPath)) {
            ORGLOG("Failed to delete temp folder on clean exit.");
        }
    }
    
    if (boost::filesystem::exists(tempDirectoryIPA)) {
        if (!boost::filesystem::remove_all(tempDirectoryIPA)) {
            ORGLOG("Failed to delete temp IPA folder on clean exit.");
        }
    }
    
    if (boost::filesystem::exists(repackedAppPath)) {
        if (!boost::filesystem::remove(repackedAppPath)) {
            ORGLOG("Failed to delete reapacked IPA file on clean exit.");
        }
    }
    
    return err;
}

