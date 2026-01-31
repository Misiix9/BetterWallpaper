#pragma once
#include "../monitor/MonitorInfo.hpp"
#include "WallpaperRenderer.hpp"
#include "WallpaperWindow.hpp"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

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
  void setMuted(bool muted);

  // Per-monitor controls
  void pause(const std::string &monitorName);
  void resume(const std::string &monitorName);
  void stop(const std::string &monitorName);

  std::string getCurrentWallpaper(const std::string &monitorName) const;

  // Called when monitors change
  void handleMonitorUpdate(const monitor::MonitorInfo &info, bool connected);

  // Persistence - save/load wallpaper state
  void saveState();
  void loadState();

  // Write to hyprpaper.conf for native persistence
  void writeHyprpaperConfig();

  // Resource Management
  void startResourceMonitor();
  void stopResourceMonitor();

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

  std::map<std::string, MonitorState> m_monitors; // Key: monitor name
  mutable std::mutex m_mutex;
  bool m_paused = false;

  // Resource Monitor
  std::thread m_monitorThread;
  std::atomic<bool> m_stopMonitor = false;
  void resourceMonitorLoop();
  void fallbackToStatic();
};

} // namespace bwp::wallpaper
