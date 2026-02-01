#pragma once
#include "WallpaperLibrary.hpp"
#include <mutex>
#include <string>
#include <vector>

namespace bwp::wallpaper {

class TagManager {
public:
  static TagManager &getInstance();

  // Tag management
  std::vector<std::string> getAllTags() const;
  std::vector<std::string> getDefaultTags() const;
  void initializeDefaultTags();           // Seeds default tags on first run
  void addTag(const std::string &tag);    // Just registers it as known
  void removeTag(const std::string &tag); // Remove from all wallpapers?

  // Assign
  void tagWallpaper(const std::string &wallpaperId, const std::string &tag);
  void untagWallpaper(const std::string &wallpaperId, const std::string &tag);

  // Fuzzy search - returns matches scored by relevance
  std::vector<std::string> fuzzyMatchTags(const std::string &query) const;

private:
  TagManager();
  ~TagManager();

  // Levenshtein distance for fuzzy matching
  static int levenshteinDistance(const std::string &s1, const std::string &s2);

  mutable std::mutex m_mutex;
  std::vector<std::string> m_knownTags; // Persistent list of tags
};

} // namespace bwp::wallpaper
