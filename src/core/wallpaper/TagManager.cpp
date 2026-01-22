#include "TagManager.hpp"
#include <algorithm>
#include <set>

namespace bwp::wallpaper {

TagManager &TagManager::getInstance() {
  static TagManager instance;
  return instance;
}

TagManager::TagManager() {}

TagManager::~TagManager() {}

std::vector<std::string> TagManager::getAllTags() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::set<std::string> tags;
  auto wallpapers = WallpaperLibrary::getInstance().getAllWallpapers();
  for (const auto &wp : wallpapers) {
    for (const auto &tag : wp.tags) {
      tags.insert(tag);
    }
  }
  return std::vector<std::string>(tags.begin(), tags.end());
}

void TagManager::addTag(const std::string &tag) {
  // No-op if we just infer from usage.
  // Unless we persist a separate tag list.
}

void TagManager::removeTag(const std::string &tag) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto all = WallpaperLibrary::getInstance().getAllWallpapers();
  for (const auto &wp : all) {
    untagWallpaper(wp.id, tag);
  }
}

void TagManager::tagWallpaper(const std::string &wallpaperId,
                              const std::string &tag) {
  auto &lib = WallpaperLibrary::getInstance();
  auto wpOpt = lib.getWallpaper(wallpaperId);
  if (wpOpt) {
    auto wp = *wpOpt;
    // Check duplicate
    if (std::find(wp.tags.begin(), wp.tags.end(), tag) == wp.tags.end()) {
      wp.tags.push_back(tag);
      lib.updateWallpaper(wp);
    }
  }
}

void TagManager::untagWallpaper(const std::string &wallpaperId,
                                const std::string &tag) {
  auto &lib = WallpaperLibrary::getInstance();
  auto wpOpt = lib.getWallpaper(wallpaperId);
  if (wpOpt) {
    auto wp = *wpOpt;
    auto it = std::remove(wp.tags.begin(), wp.tags.end(), tag);
    if (it != wp.tags.end()) {
      wp.tags.erase(it, wp.tags.end());
      lib.updateWallpaper(wp);
    }
  }
}

} // namespace bwp::wallpaper
