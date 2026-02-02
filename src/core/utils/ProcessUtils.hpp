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

    /**
     * @brief Run a command synchronously and capture output.
     * @param command The command to run (e.g., "ls -la")
     * @return Output structure with exit code, stdout, and stderr.
     */
    static Output run(const std::string& command);

    /**
     * @brief Run a command asynchronously (fire and forget).
     * @param command The command to run.
     * @return true if started successfully.
     */
    static bool runAsync(const std::string& command);

    /**
     * @brief Check if a command exists in PATH.
     * @param command Name of the executable (e.g. "mpv")
     * @return true if found.
     */
    static bool commandExists(const std::string& command);
};

} // namespace bwp::utils
