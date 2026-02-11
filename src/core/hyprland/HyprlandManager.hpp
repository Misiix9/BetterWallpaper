#pragma once
#include "../monitor/MonitorInfo.hpp"
#include "HyprlandIPC.hpp"
#include <map>
#include <mutex>
#include <string>

namespace bwp::hyprland {

class HyprlandManager {
public:
  static HyprlandManager &getInstance();

  void initialize();

  // Set wallpaper for a specific workspace
  void setWorkspaceWallpaper(int workspaceId, const std::string &wallpaperPath);
  std::string getWorkspaceWallpaper(int workspaceId) const;

  // Check if Hyprland integration is active
  bool isActive() const;

private:
  HyprlandManager();
  ~HyprlandManager();

  void onEvent(const std::string &event, const std::string &data);
  void handleWorkspaceChange(const std::string &data);
  void handleMonitorAdded(const std::string &data);

  void loadConfig();
  void saveConfig();

public:
  // Generate hyprland.conf snippet for keybinds
  std::string generateConfigSnippet() const;

private:
  std::map<int, std::string> m_workspaceWallpapers; // WorkspaceID -> Path

  std::string m_activeMonitorName; // Last active monitor
  int m_activeWorkspaceId = 1;

  mutable std::recursive_mutex m_mutex;
};

} // namespace bwp::hyprland
