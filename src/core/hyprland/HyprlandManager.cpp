#include "HyprlandManager.hpp"
#include "../config/ConfigManager.hpp"
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
    // Apply critical layer rules to prevent Hyprland interference
    // Disable animations (noanim) to stop the "Slide from top" effect
    // We use regex anchors ^()$ for strict matching
    ipc.dispatch("keyword layerrule noanim, ^(linux-wallpaperengine)$");
    ipc.dispatch("keyword layerrule noanim, ^(betterwallpaper)$");
    ipc.dispatch("keyword layerrule ignorezero, ^(linux-wallpaperengine)$");
    ipc.dispatch("keyword layerrule ignorezero, ^(betterwallpaper)$");

    ipc.setEventCallback(
        [this](const std::string &event, const std::string &data) {
          this->onEvent(event, data);
        });
  }
  loadConfig();
}
void HyprlandManager::loadConfig() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto json = conf.getJson();
  if (json.contains("hyprland")) {
    if (json["hyprland"].contains("workspaces")) {
      for (const auto &item : json["hyprland"]["workspaces"].items()) {
        try {
          int id = std::stoi(item.key());
          std::string path = item.value();
          m_workspaceWallpapers[id] = path;
        } catch (...) {
        }
      }
    }
  }
}
void HyprlandManager::saveConfig() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  nlohmann::json j;
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto &[id, path] : m_workspaceWallpapers) {
      j[std::to_string(id)] = path;
    }
  }
  conf.set("hyprland.workspaces", j);
}
bool HyprlandManager::isActive() const {
  return HyprlandIPC::getInstance().isConnected();
}
void HyprlandManager::setWorkspaceWallpaper(int workspaceId,
                                            const std::string &wallpaperPath) {
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_workspaceWallpapers[workspaceId] = wallpaperPath;
  }
  saveConfig();
}
std::string HyprlandManager::getWorkspaceWallpaper(int workspaceId) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_workspaceWallpapers.find(workspaceId);
  if (it != m_workspaceWallpapers.end()) {
    return it->second;
  }
  return "";
}
void HyprlandManager::onEvent(const std::string &event,
                              const std::string &data) {
  if (event == "workspace") {
    handleWorkspaceChange(data);
  } else if (event == "focusedmon") {
    size_t comma = data.find(',');
    if (comma != std::string::npos) {
      std::string monName = data.substr(0, comma);
      std::string wsName = data.substr(comma + 1);
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      m_activeMonitorName = monName;
      try {
        m_activeWorkspaceId = std::stoi(wsName);
        std::string wp = getWorkspaceWallpaper(m_activeWorkspaceId);
        if (!wp.empty()) {
          wallpaper::WallpaperManager::getInstance().setWallpaper(
              m_activeMonitorName, wp);
        }
      } catch (...) {
      }
    }
  } else if (event == "activewindow") {
    auto &conf = bwp::config::ConfigManager::getInstance();
    bool autoMute = conf.get<bool>("hyprland.auto_mute_on_focus", false);
    if (autoMute) {
      bool hasWindow = (data.length() > 1);
      bwp::wallpaper::WallpaperManager::getInstance().setMuted(hasWindow);
    }
  }
}
void HyprlandManager::handleWorkspaceChange(const std::string &data) {
  try {
    int workspaceId = std::stoi(data);
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_activeWorkspaceId = workspaceId;
    if (!m_activeMonitorName.empty()) {
      std::string wp = getWorkspaceWallpaper(workspaceId);
      if (!wp.empty()) {
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
  }
}
} // namespace bwp::hyprland
std::string bwp::hyprland::HyprlandManager::generateConfigSnippet() const {
  std::stringstream ss;
  ss << "# BetterWallpaper Keybinds for Hyprland\n";
  ss << "# Add these to your ~/.config/hypr/hyprland.conf\n\n";
  ss << "# Next wallpaper\n";
  ss << "bind = SUPER, N, exec, bwp next\n\n";
  ss << "# Previous wallpaper\n";
  ss << "bind = SUPER, B, exec, bwp prev\n\n";
  ss << "# Pause/Resume\n";
  ss << "bind = SUPER SHIFT, P, exec, bwp pause\n";
  ss << "bind = SUPER SHIFT, R, exec, bwp resume\n";
  return ss.str();
}
