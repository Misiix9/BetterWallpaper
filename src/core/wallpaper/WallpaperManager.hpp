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

class WallpaperWindow;
class WallpaperRenderer;
class IWallpaperSetter;
struct WallpaperInfo;

class WallpaperManager {
public:
  static WallpaperManager &getInstance();

  // Setup windows for monitors
  void initialize();

  // Set wallpaper for a specific monitor
  bool setWallpaper(const std::string &monitorName, const std::string &path);
  // Set wallpaper for multiple monitors (possibly optimizing single process)
  bool setWallpaper(const std::vector<std::string> &monitors,
                    const std::string &path);

  // Set global pause
  // Set global pause
  void setPaused(bool paused);
  void setMuted(bool muted);
  void setMuted(const std::string &monitorName,
                bool muted); // Per-monitor overload
  void setVolume(const std::string &monitorName, int volume);

  // Settings
  void setScalingMode(const std::string &monitorName, int mode);
  void setFpsLimit(int fps);
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

  std::unordered_map<std::string, MonitorState> m_monitors;

  // Forward declare to avoid include cycles or undefined types
  // Note: WallpaperWindow is already included via "WallpaperWindow.hpp",
  // so a forward declaration here is not strictly necessary for compilation.
  // If it were needed, it would typically be at the namespace scope.
  // class WallpaperWindow; // Removed to avoid shadowing/conflict

  mutable std::recursive_mutex m_mutex;  // Recursive to allow nested calls (e.g., setWallpaper -> saveState)
  bool m_paused = false;

  // Settings
  std::map<std::string, int> m_scalingModes;
  int m_fpsLimit = 60;

  // Resource Monitor
  std::thread m_monitorThread;
  std::atomic<bool> m_stopMonitor = false;
  void resourceMonitorLoop();
  void fallbackToStatic();
};

} // namespace bwp::wallpaper
