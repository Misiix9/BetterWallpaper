#pragma once
#include <cstdint>
#include <string>
namespace bwp::utils {
class SystemUtils {
public:
  static uint64_t getProcessRSS();
  static std::string formatBytes(uint64_t bytes);
  static int runCommand(const std::string &cmd);
};
}  
