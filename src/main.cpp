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
#include "signature.hpp"

bool doInjectFramework      = false;
bool doCodesign             = false;
bool doCopyEntitlements     = false;
bool doUseResourceRules     = false;
bool doRemoveEntitlements   = false;
bool doRemoveSignature      = false;
bool doUseOriginalResRules  = false;
bool doUseGenericResRules   = false;
bool doForceResRules        = false;
bool doHardened             = false;
bool isIpaFile              = false;
bool isAppFile              = false;

std::filesystem::path argTargetFilePath;
std::filesystem::path argFrameworkPath;
std::filesystem::path argCertificatePath;
std::filesystem::path argProvisionPath;
std::filesystem::path argEntitlementsPath;
std::filesystem::path argResourceRulesPath;
std::filesystem::path argDestinationPath;
std::filesystem::path argNewName;
std::filesystem::path repackedAppPath;
std::filesystem::path workingAppPath;
std::filesystem::path tempDirectoryPath;
std::filesystem::path tempDirectoryIPA;;

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
    if (vm.count("remove-signature")) {
        doRemoveSignature = true;
    }
    if (vm.count("provision")) {
        argProvisionPath = vm["provision"].as<std::string>();
    }
    if (vm.count("entitlements")) {
        argEntitlementsPath = vm["entitlements"].as<std::string>();
        doCopyEntitlements = true;
    }
    if (vm.count("resource-rules")) {
        argResourceRulesPath = vm["resource-rules"].as<std::string>();
        doUseResourceRules = true;
    }
    if (vm.count("output-dir")) {
        argDestinationPath = vm["output-dir"].as<std::string>();
    }
    if (vm.count("output-name")) {
        argNewName = vm["output-name"].as<std::string>();
    }
    if (vm.count("original-res-rules")) {
        doUseOriginalResRules = true;
    }
    if (vm.count("generic-res-rules")) {
        doUseGenericResRules = true;
    }
    if (vm.count("force-res-rules")) {
        doForceResRules = true;
    }
    if (vm.count("remove-entitlements")) {
        doRemoveEntitlements = true;
    }
    if (vm.count("remove-signature")) {
        doRemoveSignature = true;
    }
    if (vm.count("hardened")) {
        doHardened = true;
    }
    return err;
}

int check_arguments()
{
    if (doCodesign && doRemoveSignature) {
        ORGLOG("Error. Requesting remove signature and signing is incompatible.");
        return (ERR_Bad_Argument);
    }

    if (!argTargetFilePath.empty()) {
        ORGLOG_V("App to patch: " << argTargetFilePath);

        if (!std::filesystem::exists(argTargetFilePath)) {
            ORGLOG("Error: target file not found at: " << argTargetFilePath);
            return (ERR_Bad_Argument);
        }
        std::filesystem::path extension = argTargetFilePath.extension();
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
        ORGLOG_V("Framework to inject: " << argFrameworkPath);

        if (!std::filesystem::exists(argFrameworkPath)) {
            ORGLOG("Framework file not found at: " << argFrameworkPath);
            return (ERR_Bad_Argument);
        }
    }
    
    if (!argProvisionPath.empty()) {
        ORGLOG_V("Provision: " << argProvisionPath);

        if (!std::filesystem::exists(argProvisionPath)) {
            ORGLOG("Provision Profile not found at: " << argProvisionPath);
            return (ERR_Bad_Argument);
        }
    }
    
    if (!argDestinationPath.empty() && !std::filesystem::exists(argDestinationPath)) {
        ORGLOG("Destination folder does not exist: " << argDestinationPath);
        return (ERR_Bad_Argument);
    }
    return noErr;
}

int codesign_app()
{
    int err = noErr;

    ORGLOG("Codesigning the app with '" << (std::string)argCertificatePath.c_str() << "' ...");
    
    if (!argProvisionPath.empty()) {
        err = codesign_copy_provision_file(workingAppPath, argProvisionPath);
        if (err) {
            return ERR_General_Error;
        }
    }
    
    if (doRemoveEntitlements) {
        err = codesign_remove_entitlements(workingAppPath);
    }
    
    std::string systemCmd = (std::string)"/usr/bin/codesign --force -s '" + (std::string)argCertificatePath.c_str() + (std::string)"' ";
    
    // hardened
    if (doHardened) {
        systemCmd += (std::string)" --options runtime " ;
    }
    
    // Important entitlements info:
    // no entitlements provided in arguments, extract them from mobile provision
    // the provision file could be given in argument, if not extract it from the embeded.mobileprovision(ios) or embedded.provisionprofile(mac)
    std::filesystem::path entitlementsFilePath;
    
    if (doCopyEntitlements) {
        entitlementsFilePath = argEntitlementsPath;
        
        ORGLOG_V("Codesign using entitlements: " << entitlementsFilePath);
        systemCmd += (std::string)" --entitlements='" + entitlementsFilePath.string() + "'";
    } else {
        ORGLOG_V("Codesign preserving metadata.");
        systemCmd += " --preserve-metadata ";
    }
                
    std::string resRulesFile;
    if (doUseResourceRules) {
        resRulesFile = argResourceRulesPath.c_str();
    } else {
        resRulesFile = codesign_resource_rules_file(doForceResRules, doUseOriginalResRules, doUseGenericResRules, workingAppPath, tempDirectoryPath);
    }
    
    if (!resRulesFile.empty()) {
        ORGLOG_V("Codesign using resource rules file: " << (std::string)resRulesFile);
        systemCmd += (std::string)" --resource-rules='" + resRulesFile + "'";
    }
    
    systemCmd += " \"" + workingAppPath.string() + "\"";
    
    // Codesign dylibs at the first level
    if (!codesign_binaries_in_folder(workingAppPath, argCertificatePath, entitlementsFilePath)) {
        err = ERR_Dylib_Codesign_Failed;
        ORGLOG("Codesigning one or more dylibs failed");
    }
    
    // Codesigning extensions under plugins folder
    std::filesystem::path extensionsFolderPath = workingAppPath;
    extensionsFolderPath.append("Contents/PlugIns");
    if (codesign_binaries_in_folder(extensionsFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("Plugins codesign sucessful!");
    }
    
    // Codesigning frameworks
    std::filesystem::path frameworksFolderPath = workingAppPath;
    frameworksFolderPath.append("Contents/Frameworks");
    if (codesign_binaries_in_folder(frameworksFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("Frameworks codesign sucessful!");
    }

    // Codesigning Library
    std::filesystem::path libraryFolderPath = workingAppPath;
    libraryFolderPath.append("Contents/Library");
    if (codesign_binaries_in_folder(libraryFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("Frameworks codesign sucessful!");
    }

    frameworksFolderPath = workingAppPath;
    frameworksFolderPath.append("Contents/SystemFrameworks");
    if (codesign_binaries_in_folder(frameworksFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("SystemFrameworks codesign sucessful!");
    }

    frameworksFolderPath = workingAppPath;
    frameworksFolderPath.append("Contents/OtherFrameworks");
    if (codesign_binaries_in_folder(frameworksFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("OtherFrameworks codesign sucessful!");
    }

    frameworksFolderPath = workingAppPath;
    frameworksFolderPath.append("Contents/XPCServices");
    if (codesign_binaries_in_folder(frameworksFolderPath, argCertificatePath, entitlementsFilePath)) {
        ORGLOG_V("XPCServices codesign sucessful!");
    }

    if (!silenceCmdOutput.empty()) {
        systemCmd += SPACE + silenceCmdOutput;
    }

    // sign the app
    err = system((const char*)systemCmd.c_str());
    if (err) {
        ORGLOG("Code signing app failed. Cmd: " << systemCmd);
        return  ERR_App_Codesign_Failed;
    }

    return err;
}

int prepare_folders()
{
    int err = 0;
    
    // Create temp folder path
    tempDirectoryPath = std::filesystem::temp_directory_path() / "macho_inject";

    // Remove old temp if exist
    ORGLOG_V("Cleaning temp folders ...");
    if (std::filesystem::exists(tempDirectoryPath)) {
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
    if (std::filesystem::exists(tempDirectoryIPA)) {
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
        std::filesystem::path appName = std::filesystem::path(cmdResult).filename();
        std::filesystem::path appNameWithoutExtension = std::filesystem::path(cmdResult).filename().replace_extension("");
        workingAppPath = tempDirectoryPath;
        workingAppPath.append("Payload").append(appName.string());
    } else if (isAppFile) {
        ORGLOG_V("Copying App to temp working folder ...");
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

int deploy_app_to_destination()
{
    int err = noErr;
    
    if (isIpaFile) {
        err = repack(tempDirectoryPath, repackedAppPath);
        if (err) {
            ORGLOG("Error. Failed to zip ipa.");
        } else {
            std::filesystem::path newAppPath;
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
            ORGLOG_V("Copying App to destination folder ...");
            std::filesystem::path finalAppPath;
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
    if (err != noErr) {
        ORGLOG("Use --help.");
        exit(err);
    }

    err = prepare_folders();
    if (err != noErr)
        exit(err);

    if (doInjectFramework) {
        err = inj_inject_framework_into_app(workingAppPath, argFrameworkPath, doRemoveSignature);
        if (err != noErr)
            goto CLEAN_EXIT;
    }
    
    if (doCodesign) {
        err = codesign_app();
        if (err != noErr)
            exit(err);
    }

    if (doRemoveSignature) {
        err = codesign_remove_signature_from_app(workingAppPath);
        if (err != noErr)
            goto CLEAN_EXIT;
    }

    err = deploy_app_to_destination();
        
    
CLEAN_EXIT:
    
    if (std::filesystem::exists(tempDirectoryPath)) {
        std::error_code error;
        if (!std::filesystem::remove_all(tempDirectoryPath, error)) {
            ORGLOG("Failed to delete temp folder on clean exit. " << error.message());
        }
    }
    
    if (std::filesystem::exists(tempDirectoryIPA)) {
        std::error_code error;
        if (!std::filesystem::remove_all(tempDirectoryIPA)) {
            ORGLOG("Failed to delete temp IPA folder on clean exit. " << error.message());
        }
    }
    
    if (std::filesystem::exists(repackedAppPath)) {
        std::error_code error;
        if (!std::filesystem::remove(repackedAppPath)) {
            ORGLOG("Failed to delete reapacked IPA file on clean exit. " << error.message());
        }
    }
    
    return err;
}

