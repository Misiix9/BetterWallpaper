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
  void addTag(const std::string &tag);    // Just registers it as known
  void removeTag(const std::string &tag); // Remove from all wallpapers?

  // Assign
  void tagWallpaper(const std::string &wallpaperId, const std::string &tag);
  void untagWallpaper(const std::string &wallpaperId, const std::string &tag);

private:
  TagManager();
  ~TagManager();

  mutable std::mutex m_mutex;
  // We can infer tags from usage in wallpapers, or store separate list.
  // Storing separate list allows empty tags.
  // implementation simplification: infer from usage for now + dedicated config
  // list?
};

} // namespace bwp::wallpaper
