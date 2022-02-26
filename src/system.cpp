//
//  system.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 28/02/2017.
//

#include <fstream>
#include "system.hpp"


/**
 Executes a system command described in the argument. It read the output of the command and returns it.
 THis function is helpful when we need to read the result of a sys command.

 @param commandToRun An string with the sytem command to be executed.
 @return The result of the execution. Only first line is returned.
 */
std::string run_command (const std::string& commandToRun)
{
    char fileLine[4096] = "";
    std::filesystem::path tempFileFullPath = std::filesystem::temp_directory_path();
    tempFileFullPath.append("mcTempFileForSystemCmd");
    std::string cmd = commandToRun + " >> " + tempFileFullPath.string();
    int err = std::system(cmd.c_str());
    if (err == 0) {
        std::ifstream file(tempFileFullPath.c_str(), std::ios::in );
        file.getline(fileLine, sizeof(fileLine)); // Only interested in the first line. Will not read the CR LF, which can make the caller's life easier.
        file.close();
    } else {
        ORGLOG("Error executing system command: " + commandToRun);
        if(err != 32512) {
            exit(1);
        }
    }
    err = remove(tempFileFullPath.c_str());
    if (err) {
        ORGLOG("Error removing temp file: " + tempFileFullPath.string());
    }
    return std::string(fileLine);
}
