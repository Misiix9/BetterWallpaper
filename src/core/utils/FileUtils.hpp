#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace bwp::utils {

class FileUtils {
public:
  static bool exists(const std::filesystem::path &path);
  static bool createDirectories(const std::filesystem::path &path);
  static std::string readFile(const std::filesystem::path &path);
  static bool writeFile(const std::filesystem::path &path,
                        const std::string &content);

  // Expands ~ to home directory
  static std::filesystem::path expandPath(const std::string &pathVal);

  static std::string getMimeType(const std::filesystem::path &path);
  static std::string calculateHash(const std::filesystem::path &path); // SHA256

  static std::string getExtension(const std::filesystem::path &path);
};

} // namespace bwp::utils
