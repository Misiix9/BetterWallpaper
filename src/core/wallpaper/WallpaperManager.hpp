#pragma once
#include "../monitor/MonitorInfo.hpp"
#include "WallpaperRenderer.hpp"
#include "WallpaperWindow.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace bwp::wallpaper {

class WallpaperManager {
public:
  static WallpaperManager &getInstance();

  // Setup windows for monitors
  void initialize();

  // Set wallpaper for a specific monitor
  bool setWallpaper(const std::string &monitorName, const std::string &path);

  // Set global pause
  void setPaused(bool paused);

  std::string getCurrentWallpaper(const std::string &monitorName) const;

  // Called when monitors change
  void handleMonitorUpdate(const monitor::MonitorInfo &info, bool connected);

private:
  WallpaperManager();
  ~WallpaperManager();

  std::shared_ptr<WallpaperRenderer> createRenderer(const std::string &path);

  struct MonitorState {
    std::shared_ptr<WallpaperWindow> window;
    std::shared_ptr<WallpaperRenderer> renderer;
    std::string currentPath;
  };

  void killConflictingWallpapers();

  mutable std::mutex m_mutex;
  std::map<std::string, MonitorState> m_monitors; // Key: monitor name
  bool m_paused = false;
};

} // namespace bwp::wallpaper
