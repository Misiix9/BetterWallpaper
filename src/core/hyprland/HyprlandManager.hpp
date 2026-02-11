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
  void setWorkspaceWallpaper(int workspaceId, const std::string &wallpaperPath);
  std::string getWorkspaceWallpaper(int workspaceId) const;
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
  std::string generateConfigSnippet() const;
private:
  std::map<int, std::string> m_workspaceWallpapers;  
  std::string m_activeMonitorName;  
  int m_activeWorkspaceId = 1;
  mutable std::recursive_mutex m_mutex;
};
}  
