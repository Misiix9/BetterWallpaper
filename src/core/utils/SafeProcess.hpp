#pragma once
#include <string>
#include <vector>
namespace bwp::utils {
class SafeProcess {
public:
  struct Output {
    int exitCode;
    std::string stdOut;
    std::string stdErr;
    bool success() const { return exitCode == 0; }
  };
  static Output exec(const std::vector<std::string> &args);
  static Output exec(const std::vector<std::string> &args,
                     const std::vector<std::string> &envVars);
  static bool execDetached(const std::vector<std::string> &args,
                           const std::vector<std::string> &envVars = {});
  static bool commandExists(const std::string &name);
  static std::string shellEscape(const std::string &input);
};
}  
