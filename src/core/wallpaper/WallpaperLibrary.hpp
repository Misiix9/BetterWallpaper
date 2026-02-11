#pragma once
#include "WallpaperInfo.hpp"
#include <filesystem>
#include <mutex>
#include <atomic>
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
  void updateBlurhash(const std::string &id, const std::string &hash);
  void removeWallpaper(const std::string &id);

  std::optional<WallpaperInfo> getWallpaper(const std::string &id) const;
  std::vector<WallpaperInfo> getAllWallpapers() const;
  std::vector<std::string> getAllTags() const;

  // Query
  std::vector<WallpaperInfo> search(const std::string &query) const;
  std::vector<WallpaperInfo>
  filter(const std::function<bool(const WallpaperInfo &)> &predicate) const;

  // Signals
  using ChangeCallback = std::function<void(const WallpaperInfo &info)>;
  void setChangeCallback(ChangeCallback cb); // Deprecated: use addChangeCallback
  void addChangeCallback(ChangeCallback cb);

  // Data directory access
  std::filesystem::path getDataDirectory() const;

private:
  void removeDuplicates();

  WallpaperLibrary();
  ~WallpaperLibrary();

  void load();
  std::filesystem::path getDatabasePath() const;

  std::unordered_map<std::string, WallpaperInfo> m_wallpapers;
  std::unordered_map<std::string, std::string> m_pathToId;
  mutable std::recursive_mutex m_mutex;
  std::filesystem::path m_dbPath;
  bool m_dirty = false;
  std::atomic<bool> m_initialized{false};
  ChangeCallback m_changeCallback;
  std::vector<ChangeCallback> m_changeCallbacks;
};

} // namespace bwp::wallpaper
