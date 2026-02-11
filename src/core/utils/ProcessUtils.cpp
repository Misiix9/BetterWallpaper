#include "ProcessUtils.hpp"
#include "Logger.hpp"
#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstdlib>
namespace bwp::utils {
ProcessUtils::Output ProcessUtils::run(const std::string& command) {
    std::string data;
    std::string cmd = command + " 2>&1";  
    const int bufferSize = 128;
    std::vector<char> buffer(bufferSize);
#ifdef _WIN32
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif
    std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(cmd.c_str(), "r"), PCLOSE);
    if (!pipe) {
        LOG_ERROR("ProcessUtils::run failed to open pipe for: " + command);
        return { -1, "", "Failed to open pipe" };
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        data += buffer.data();
    }
    int rc = PCLOSE(pipe.release());
    if (!data.empty() && data.back() == '\n') {
        data.pop_back();
    }
    return { rc, data, "" };
}
bool ProcessUtils::runAsync(const std::string& command) {
    std::string cmd = command;
#ifdef _WIN32
#else
    cmd += " &";  
#endif
    int rc = std::system(cmd.c_str());
    return rc == 0;
}
bool ProcessUtils::commandExists(const std::string& command) {
    std::string checkCmd;
#ifdef _WIN32
    checkCmd = "where " + command + " > nul 2>&1";
#else
    checkCmd = "which " + command + " > /dev/null 2>&1";
#endif
    return std::system(checkCmd.c_str()) == 0;
}
}  
