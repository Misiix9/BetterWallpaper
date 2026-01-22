#include "FileUtils.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <unistd.h>

namespace bwp::utils {

bool FileUtils::exists(const std::filesystem::path &path) {
  return std::filesystem::exists(path);
}

bool FileUtils::createDirectories(const std::filesystem::path &path) {
  try {
    return std::filesystem::create_directories(path);
  } catch (...) {
    return false;
  }
}

std::string FileUtils::readFile(const std::filesystem::path &path) {
  std::ifstream f(path);
  if (!f.is_open())
    return "";
  std::stringstream buffer;
  buffer << f.rdbuf();
  return buffer.str();
}

bool FileUtils::writeFile(const std::filesystem::path &path,
                          const std::string &content) {
  // Create parent if needed
  if (path.has_parent_path()) {
    createDirectories(path.parent_path());
  }
  std::ofstream f(path);
  if (!f.is_open())
    return false;
  f << content;
  return true;
}

std::filesystem::path FileUtils::expandPath(const std::string &pathVal) {
  if (pathVal.empty())
    return "";

  if (pathVal[0] == '~') {
    const char *home = std::getenv("HOME");
    if (!home) {
      struct passwd *pw = getpwuid(getuid());
      if (pw)
        home = pw->pw_dir;
    }
    if (home) {
      std::string result = home;
      if (pathVal.length() > 1 && pathVal[1] == '/') {
        result += pathVal.substr(1);
      } else if (pathVal.length() > 1) {
        // ~user syntax not supported fully here, simplified
        result += "/" + pathVal.substr(1);
      }
      return std::filesystem::path(result);
    }
  }
  return std::filesystem::path(pathVal);
}

std::string FileUtils::getMimeType(const std::filesystem::path &path) {
  // Basic extension check for now, can implement magic bytes later
  // or use 'file --mime-type' command

  std::string ext = getExtension(path);
  for (auto &c : ext)
    c = std::tolower(c);

  if (ext == "png")
    return "image/png";
  if (ext == "jpg" || ext == "jpeg")
    return "image/jpeg";
  if (ext == "webm")
    return "video/webm";
  if (ext == "mp4")
    return "video/mp4";
  if (ext == "gif")
    return "image/gif";
  if (ext == "pkg")
    return "application/x-wallpaper-engine";

  return "application/octet-stream";
}

std::string FileUtils::calculateHash(const std::filesystem::path &path) {
  // Using simple shell command for now to avoid OpenSSL dependency complexity
  // in this phase Sha256sum
  std::string cmd = "sha256sum \"" + path.string() + "\" 2>/dev/null";
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    return "";
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  // Result format: "hash  filename"
  std::istringstream iss(result);
  std::string hash;
  iss >> hash;
  return hash;
}

std::string FileUtils::getExtension(const std::filesystem::path &path) {
  if (!path.has_extension())
    return "";
  std::string ext = path.extension().string();
  if (!ext.empty() && ext[0] == '.')
    ext = ext.substr(1);
  return ext;
}

} // namespace bwp::utils
