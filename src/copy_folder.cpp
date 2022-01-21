//
//  copy_folder.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 3/15/17.
//

#include "copy_folder.hpp"


bool copy_app(const boost::filesystem::path &appPath,  const boost::filesystem::path &destinationPath, boost::filesystem::path &finalAppPath)
{
    const std::string SPACE = " ";
    const std::string QUOTE = "\"";

    boost::filesystem::path bundleName = appPath.filename();
    finalAppPath = destinationPath / bundleName;
    
    std::string systemCmd = (std::string)"/bin/cp -fa " + QUOTE + appPath.string() + QUOTE + SPACE + finalAppPath.string();
    int err = system(systemCmd.c_str());
    if (err != noErr) {
        ORGLOG("Error: Failed target app to folder: " << finalAppPath);
        return false;
    }
    return true;
}

bool copy_bundle(const boost::filesystem::path &bundlePath,  const boost::filesystem::path &destinationPath)
{
    const std::string SPACE = " ";
    const std::string QUOTE = "\"";

    // System "cp" does not create the App folder, we need to create it.
    
    boost::filesystem::path bundleName = bundlePath.filename();
    boost::filesystem::path finalDirPath = destinationPath / bundleName;

    boost::system::error_code error;
    boost::filesystem::copy_directory(bundlePath, destinationPath, error);
    if (error) {
        ORGLOG("Error copying directory. " << bundlePath << " to " << destinationPath << " Error: " << error.message());
        return false;
    }
    
    std::string systemCmd = (std::string)"/bin/cp -fa " + QUOTE + bundlePath.string() + QUOTE + SPACE + finalDirPath.string();
    int err = system(systemCmd.c_str());
    if (err != noErr) {
        ORGLOG("Error: Failed copying bundle to folder: " << finalDirPath);
        return false;
    }
    return true;
}

/**
 Generic function to copy/duplicate a directory and its contents into a new destination.
 It is not recursive copy. Copies only first level.

 @param sourceDirPath The folder to copy
 @param destDirPath Destination folder where the new folder will be created.
 @return 0 if success.
 */
/*
int copy_folder(const boost::filesystem::path & sourceDirPath, const boost::filesystem::path & destDirPath)
{
    ORGLOG_V("Copying JS folder inside IPA.: " << sourceDirPath);
    
    if (boost::filesystem::exists(sourceDirPath) == false) {
        ORGLOG("Error the origin folder does not exists : " << sourceDirPath);
        return -1;
    }
    
    // Delete destination folder
    boost::filesystem::path newFolderPath(destDirPath);
    newFolderPath.append(JSFolder);
    if (boost::filesystem::exists(newFolderPath)) {
        boost::system::error_code error;
        boost::filesystem::remove_all(newFolderPath, error);
        if (error.value()) {
            ORGLOG("Error deleting the destination folder: " << error.message());
        }
    }
    
    // Create destination folder.
    if (!boost::filesystem::create_directory(newFolderPath)) {
        ORGLOG("Error creating JS Libs folder in the IPA: " << newFolderPath);
        return -1;
    }
    
    // copy_directory does not copy the contents !
//    boost::system::error_code error;
//    boost::filesystem::copy_directory(sourceDirPath, newFolderPath, error);
//    if (error.value()) {
//        ORGLOG("Error copying folder: " << error.message());
//        return error.value();
//    }
    
    // Iterate over all files and copy them into the new folder
    for (boost::filesystem::directory_iterator itr(sourceDirPath); itr!=boost::filesystem::directory_iterator(); ++itr) {
       if(boost::filesystem::is_regular_file(itr->path())) {
            boost::filesystem::copy_file(itr->path(), newFolderPath / itr->path().filename());
        }

    }
    return 0;
}
*/
