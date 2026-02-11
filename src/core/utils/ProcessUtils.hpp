#pragma once
#include <string>
#include <vector>
#include <optional>
namespace bwp::utils {
class ProcessUtils {
public:
    struct Output {
        int exitCode;
        std::string stdOut;
        std::string stdErr;
        bool success() const { return exitCode == 0; }
    };
    static Output run(const std::string& command);
    static bool runAsync(const std::string& command);
    static bool commandExists(const std::string& command);
};
}  
