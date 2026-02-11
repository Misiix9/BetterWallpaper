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
  void initialize();
  bool setWallpaper(const std::string &monitorName, const std::string &path);
  bool setWallpaper(const std::vector<std::string> &monitors,
                    const std::string &path);
  void setPaused(bool paused);
  void setMuted(bool muted);
  void setMuted(const std::string &monitorName,
                bool muted);  
  void setVolume(const std::string &monitorName, int volume);
  void setScalingMode(const std::string &monitorName, int mode);
  void setFpsLimit(int fps);
  void setNoAudioProcessing(bool enabled);
  void setDisableMouse(bool enabled);
  void setNoAutomute(bool enabled);
  void setVolumeLevel(int volume);
  void pause(const std::string &monitorName);
  void resume(const std::string &monitorName);
  void stop(const std::string &monitorName);
  std::string getCurrentWallpaper(const std::string &monitorName) const;
  void handleMonitorUpdate(const monitor::MonitorInfo &info, bool connected);
  void saveState();
  void loadState();
  void writeHyprpaperConfig();
  void startResourceMonitor();
  void stopResourceMonitor();
  void shutdown();
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
  mutable std::recursive_mutex m_mutex;  
  bool m_paused = false;
  std::map<std::string, int> m_scalingModes;
  int m_fpsLimit = 60;
  bool m_globalMuted = false;
  bool m_noAudioProcessing = false;
  bool m_disableMouse = false;
  bool m_noAutomute = false;
  int m_volumeLevel = 50;
  std::thread m_monitorThread;
  std::atomic<bool> m_stopMonitor = false;
  void resourceMonitorLoop();
  void fallbackToStatic();
};
}  
