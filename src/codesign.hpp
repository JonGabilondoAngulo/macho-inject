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

bool codesign_binary(const std::filesystem::path & appPath, const std::string& dllName, const std::filesystem::path& certificateFilePath, const std::filesystem::path& entitlementsFilePath);
bool codesign_at_path(const std::filesystem::path & appPath, const std::filesystem::path&  certificateFilePath, const std::filesystem::path&  entitlementsFilePath);
int remove_codesign(const std::filesystem::path& appPath);

#pragma mark - Entitlements functions

int remove_entitlements(const std::filesystem::path& appPath);
bool extract_entitlements_from_mobile_provision(const std::string& provisionFile, std::filesystem::path& outNewEntitlementsFile, const std::filesystem::path & tempDirPath);
bool extract_entitlements_from_app(const std::string& appPath, std::string& newEntitlementsFile, const std::string& tempDirPath);

#pragma mark - Provision functions

int copy_provision_file(const std::filesystem::path & appPath, const std::filesystem::path& argProvision);

#pragma mark - Resource rules functions

std::string resource_rules_file(bool forceResRules, bool useOriginalResRules, bool useGenericResRules, const std::filesystem::path & appPath, const std::filesystem::path & tempDirPath);

#pragma mark - Validation functions

bool verify_app_correctness(const std::filesystem::path& appPath, const std::string& dllName);

#endif /* codesign_hpp */
