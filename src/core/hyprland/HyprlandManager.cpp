#include "HyprlandManager.hpp"
#include "../utils/Logger.hpp"
#include "../wallpaper/WallpaperManager.hpp"
#include <iostream>
#include <sstream>

namespace bwp::hyprland {

HyprlandManager &HyprlandManager::getInstance() {
  static HyprlandManager instance;
  return instance;
}

HyprlandManager::HyprlandManager() {}

HyprlandManager::~HyprlandManager() {}

void HyprlandManager::initialize() {
  auto &ipc = HyprlandIPC::getInstance();
  if (ipc.connect()) {
    ipc.setEventCallback(
        [this](const std::string &event, const std::string &data) {
          this->onEvent(event, data);
        });

    // Initial state sync could be done here (querying monitors/workspaces)
    // For now, reliance on events is sufficient for dynamic changes
  }
}

bool HyprlandManager::isActive() const {
  return HyprlandIPC::getInstance().isConnected();
}

void HyprlandManager::setWorkspaceWallpaper(int workspaceId,
                                            const std::string &wallpaperPath) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_workspaceWallpapers[workspaceId] = wallpaperPath;
  // If currently active workspace is this one, apply immediately?
  // Logic complicates here without full state tracking, let's assume next
  // switch handles it
}

std::string HyprlandManager::getWorkspaceWallpaper(int workspaceId) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_workspaceWallpapers.find(workspaceId);
  if (it != m_workspaceWallpapers.end()) {
    return it->second;
  }
  return "";
}

void HyprlandManager::onEvent(const std::string &event,
                              const std::string &data) {
  // LOG_DEBUG("Hyprland Event: " + event + " >> " + data);

  if (event == "workspace") {
    // data is WORKSPACENAME
    handleWorkspaceChange(data);
  } else if (event == "focusedmon") {
    // data is MONNAME,WORKSPACENAME
    size_t comma = data.find(',');
    if (comma != std::string::npos) {
      std::string monName = data.substr(0, comma);
      std::string wsName = data.substr(comma + 1);

      std::lock_guard<std::mutex> lock(m_mutex);
      m_activeMonitorName = monName;

      // Treat as workspace change to update wallpaper if needed
      // Try to parse ID from name if it's a number, else we need a map Name->ID
      try {
        m_activeWorkspaceId = std::stoi(wsName);
        std::string wp = getWorkspaceWallpaper(m_activeWorkspaceId);
        if (!wp.empty()) {
          wallpaper::WallpaperManager::getInstance().setWallpaper(
              m_activeMonitorName, wp);
        }
      } catch (...) {
        // Ignore non-numeric workspaces for now
      }
    }
  }
}

void HyprlandManager::handleWorkspaceChange(const std::string &data) {
  try {
    int workspaceId = std::stoi(data);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeWorkspaceId = workspaceId;

    // Apply wallpaper for this workspace to the active monitor
    // Using activeMonitorName which should have been set by focusedmon
    // previously or init If empty, we might not know which monitor.

    if (!m_activeMonitorName.empty()) {
      std::string wp = getWorkspaceWallpaper(workspaceId);
      if (!wp.empty()) {
        // Check if it's already the current one to avoid reloading same
        // wallpaper
        std::string current =
            wallpaper::WallpaperManager::getInstance().getCurrentWallpaper(
                m_activeMonitorName);
        if (current != wp) {
          wallpaper::WallpaperManager::getInstance().setWallpaper(
              m_activeMonitorName, wp);
        }
      }
    }

  } catch (...) {
    // Workspace name might not be number
  }
}

} // namespace bwp::hyprland
