#pragma once
#include "WallpaperLibrary.hpp"
#include <mutex>
#include <string>
#include <vector>
namespace bwp::wallpaper {
class TagManager {
public:
  static TagManager &getInstance();
  std::vector<std::string> getAllTags() const;
  std::vector<std::string> fuzzyMatchTags(const std::string &query) const;
private:
  TagManager();
  ~TagManager();
  static int levenshteinDistance(const std::string &s1, const std::string &s2);
  mutable std::mutex m_mutex;
};
}  
