//
//  codesign.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#ifndef codesign_hpp
#define codesign_hpp

#include <stdio.h>

#pragma mark - Codesign functions

bool codesign_fs_object(const std::filesystem::path & folderPath, const std::string& fsObjectName, const std::filesystem::path& certificateFilePath, const std::filesystem::path& entitlementsFilePath);
bool codesign_binaries_in_folder(const std::filesystem::path & folderPath, const std::filesystem::path&  certificateFilePath, const std::filesystem::path&  entitlementsFilePath);
int codesign_remove_signature(const std::filesystem::path& appPath);

#pragma mark - Entitlements functions

int codesign_remove_entitlements(const std::filesystem::path& appPath);
bool codesign_extract_entitlements_from_mobile_provision(const std::string& provisionFile, std::filesystem::path& outNewEntitlementsFile, const std::filesystem::path & tempDirPath);
bool codesign_extract_entitlements_from_app(const std::string& appPath, std::string& newEntitlementsFile, const std::string& tempDirPath);

#pragma mark - Provision functions

int codesign_copy_provision_file(const std::filesystem::path & appPath, const std::filesystem::path& argProvision);

#pragma mark - Resource rules functions

std::string codesign_resource_rules_file(bool forceResRules, bool useOriginalResRules, bool useGenericResRules, const std::filesystem::path & appPath, const std::filesystem::path & tempDirPath);

#pragma mark - Validation functions

bool codesign_verify_app_correctness(const std::filesystem::path& appPath, const std::string& dllName);

#endif /* codesign_hpp */
