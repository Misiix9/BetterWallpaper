#pragma once
#include <cstdint>
#include <string>

namespace bwp::utils {

class SystemUtils {
public:
  // Returns Resident Set Size (RSS) in bytes
  static uint64_t getProcessRSS();

  // Format bytes to human readable string (e.g. "1.5 GB")
  static std::string formatBytes(uint64_t bytes);

  // Run shell command synchronously, returns exit code
  static int runCommand(const std::string &cmd);
};

} // namespace bwp::utils
