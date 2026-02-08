#pragma once
#include "WallpaperLibrary.hpp"
#include <mutex>
#include <string>
#include <vector>

namespace bwp::wallpaper {

class TagManager {
public:
  static TagManager &getInstance();

  // Tag management — tags are auto-discovered from project.json only
  std::vector<std::string> getAllTags() const;

  // Fuzzy search - returns matches scored by relevance
  std::vector<std::string> fuzzyMatchTags(const std::string &query) const;

private:
  TagManager();
  ~TagManager();

  // Levenshtein distance for fuzzy matching
  static int levenshteinDistance(const std::string &s1, const std::string &s2);

  mutable std::mutex m_mutex;
};

} // namespace bwp::wallpaper
