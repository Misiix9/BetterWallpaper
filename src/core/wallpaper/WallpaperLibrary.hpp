#pragma once
#include "WallpaperInfo.hpp"
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace bwp::wallpaper {

class WallpaperLibrary {
public:
  static WallpaperLibrary &getInstance();

  void initialize();
  void save();

  // CRUD
  void addWallpaper(const WallpaperInfo &info);
  void updateWallpaper(const WallpaperInfo &info);
  void removeWallpaper(const std::string &id);

  std::optional<WallpaperInfo> getWallpaper(const std::string &id) const;
  std::vector<WallpaperInfo> getAllWallpapers() const;

  // Query
  std::vector<WallpaperInfo> search(const std::string &query) const;
  std::vector<WallpaperInfo>
  filter(const std::function<bool(const WallpaperInfo &)> &predicate) const;

private:
  WallpaperLibrary();
  ~WallpaperLibrary();

  void load();
  std::filesystem::path getDatabasePath() const;

  std::unordered_map<std::string, WallpaperInfo> m_wallpapers;
  mutable std::mutex m_mutex;
  std::filesystem::path m_dbPath;
  bool m_dirty = false;
};

} // namespace bwp::wallpaper
