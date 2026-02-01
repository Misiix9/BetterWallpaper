#include "WallpaperManager.hpp"
#include "../config/ConfigManager.hpp"
#include "../hyprland/HyprlandManager.hpp"
#include "../monitor/MonitorManager.hpp"
#include "../notification/NotificationManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SystemUtils.hpp"
#include "NativeWallpaperSetter.hpp"
#include "renderers/StaticRenderer.hpp"
#include "renderers/VideoRenderer.hpp"
#include "renderers/WallpaperEngineRenderer.hpp"
#include <cstdlib>
#include <fstream>

namespace bwp::wallpaper {

WallpaperManager &WallpaperManager::getInstance() {
  static WallpaperManager instance;
  return instance;
}

WallpaperManager::WallpaperManager() {
  // Connect to monitor manager callback
  monitor::MonitorManager::getInstance().setCallback(
      [this](const monitor::MonitorInfo &info, bool connected) {
        this->handleMonitorUpdate(info, connected);
      });
}

WallpaperManager::~WallpaperManager() {
  // Cleanup
}

void WallpaperManager::initialize() {
  monitor::MonitorManager::getInstance().initialize();
  auto monitors = monitor::MonitorManager::getInstance().getMonitors();
  for (const auto &m : monitors) {
    handleMonitorUpdate(m, true);
  }

  // Initialize Hyprland integration
  ::bwp::hyprland::HyprlandManager::getInstance().initialize();

  // Start Resource Monitor
  startResourceMonitor();
}

void WallpaperManager::handleMonitorUpdate(const monitor::MonitorInfo &info,
                                           bool connected) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (connected) {
    if (m_monitors.find(info.name) == m_monitors.end()) {
      LOG_INFO("Initializing wallpaper window for monitor: " + info.name);
      MonitorState state;
      state.window = std::make_shared<WallpaperWindow>(info);
      state.window->show();
      m_monitors[info.name] = state;
    }
  } else {
    if (m_monitors.find(info.name) != m_monitors.end()) {
      LOG_INFO("Removing wallpaper window for monitor: " + info.name);
      m_monitors.erase(info.name);
    }
  }
}

bool WallpaperManager::setWallpaper(const std::string &monitorName,
                                    const std::string &path) {
  LOG_INFO("WallpaperManager::setWallpaper called for " + monitorName +
           " with path: " + path);

  std::lock_guard<std::mutex> lock(m_mutex);

  LOG_INFO("Currently have " + std::to_string(m_monitors.size()) + " monitors");

  // Find monitor state
  auto it = m_monitors.find(monitorName);
  if (it == m_monitors.end()) {
    LOG_ERROR("Monitor not found: " + monitorName + ". Available monitors:");
    for (const auto &[name, state] : m_monitors) {
      LOG_ERROR("  - " + name);
    }
    return false;
  }

  LOG_INFO("Found monitor " + monitorName);

  // Determine type and create renderer
  auto renderer = createRenderer(path);
  if (!renderer) {
    LOG_ERROR("Failed to create renderer for path: " + path);
    return false;
  }

  LOG_INFO("Created renderer for wallpaper");

  renderer->setMonitor(monitorName);

  if (!renderer->load(path)) {
    LOG_ERROR("Failed to load wallpaper: " + path);
    return false;
  }

  LOG_INFO("Loaded wallpaper successfully");

  // Stop old renderer
  if (it->second.renderer) {
    LOG_INFO("Stopping old renderer");
    it->second.renderer->stop();
  }

  it->second.renderer = renderer;
  it->second.currentPath = path;

  LOG_INFO("Transitioning to new wallpaper...");
  it->second.window->transitionTo(renderer);

  if (m_paused) {
    renderer->pause();
  } else {
    LOG_INFO("Starting wallpaper playback");
    renderer->play();
  }

  // Kill other wallpaper managers NOW, to overwrite them visually
  killConflictingWallpapers();

  // For static images, also use native setter as backup (swaybg, etc.)
  // This ensures the wallpaper is visible even if our window has issues
  std::string mime = utils::FileUtils::getMimeType(path);
  std::string ext = utils::FileUtils::getExtension(path);
  bool isStaticImage = (mime.find("image/") != std::string::npos &&
                        mime.find("gif") == std::string::npos) ||
                       (ext == "png" || ext == "jpg" || ext == "jpeg" ||
                        ext == "webp" || ext == "bmp");

  if (isStaticImage) {
    LOG_INFO("Using native wallpaper setter for static image: " + path);
    NativeWallpaperSetter::getInstance().setWallpaper(path, monitorName);
  }

  LOG_INFO("Wallpaper set successfully!");

  // Save state for persistence (without app running)
  saveState();
  writeHyprpaperConfig();

  return true;
}

std::shared_ptr<WallpaperRenderer>
WallpaperManager::createRenderer(const std::string &path) {
  // Detect type by extension from FileUtils
  std::string mime = utils::FileUtils::getMimeType(path);

  if (mime.find("image/") != std::string::npos &&
      mime.find("gif") == std::string::npos) {
    return std::make_shared<StaticRenderer>();
  } else if (mime.find("video/") != std::string::npos ||
             mime.find("gif") != std::string::npos) {
    return std::make_shared<VideoRenderer>(); // MPV handles gif
  } else if (mime.find("x-wallpaper-engine") != std::string::npos) {
    return std::make_shared<WallpaperEngineRenderer>();
  }

  // Fallback based on extension if mime detection is weak
  std::string ext = utils::FileUtils::getExtension(path);
  if (ext == "mp4" || ext == "webm" || ext == "mkv" || ext == "gif") {
    return std::make_shared<VideoRenderer>();
  }
  if (ext == "pkg" || ext == "json") { // .json often project.json for WE
    return std::make_shared<WallpaperEngineRenderer>();
  }

  return std::make_shared<StaticRenderer>(); // Default
}

void WallpaperManager::setPaused(bool paused) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_paused = paused;
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      if (paused)
        state.renderer->pause();
      else
        state.renderer->play();
    }
  }
}

void WallpaperManager::setMuted(bool muted) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      state.renderer->setMuted(muted);
    }
  }
}

void WallpaperManager::pause(const std::string &monitorName) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end() && it->second.renderer) {
    it->second.renderer->pause();
  }
}

void WallpaperManager::resume(const std::string &monitorName) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  // If global pause is on, resuming single monitor might be conflicted
  // But usually Resume command implies override.
  if (it != m_monitors.end() && it->second.renderer) {
    it->second.renderer->play();
  }
}

void WallpaperManager::stop(const std::string &monitorName) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end() && it->second.renderer) {
    it->second.renderer->stop();
  }
}

std::string
WallpaperManager::getCurrentWallpaper(const std::string &monitorName) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end()) {
    return it->second.currentPath;
  }
  return "";
}

void WallpaperManager::killConflictingWallpapers() {
  // List of known wallpaper managers to kill
  const std::vector<std::string> conflicts = {
      "hyprpaper",  "swaybg",   "swww-daemon", "mpvpaper",    "wpaperd",
      "waypaper",   "aztime",   "glpaper",     "feh",         "nitrogen",
      "xwallpaper", "hsetroot", "habak",       "displayball", "eww-daemon",
      "ags",        "swww",     "mpv"};

  for (const auto &proc : conflicts) {
    // Aggressive kill
    std::string cmd = "pkill -9 -x " + proc + " 2>/dev/null";
    system(cmd.c_str());
    // Also try without -x for partial matches if needed, but -x is safer for
    // exact matches like 'mpv' vs 'mpvpaper'
  }
}

void WallpaperManager::saveState() {
  auto &conf = bwp::config::ConfigManager::getInstance();

  std::lock_guard<std::mutex> lock(m_mutex);
  nlohmann::json wallpapers;

  for (const auto &[name, state] : m_monitors) {
    if (!state.currentPath.empty()) {
      wallpapers[name] = state.currentPath;
    }
  }

  conf.set("wallpapers.current", wallpapers);
  LOG_INFO("Saved wallpaper state for " + std::to_string(wallpapers.size()) +
           " monitors");
}

void WallpaperManager::loadState() {
  auto &conf = bwp::config::ConfigManager::getInstance();

  auto wallpapers = conf.get<nlohmann::json>("wallpapers.current");
  if (wallpapers.is_object()) {
    for (auto &[monitor, path] : wallpapers.items()) {
      if (path.is_string()) {
        LOG_INFO("Restoring wallpaper for " + monitor + ": " +
                 path.get<std::string>());
        setWallpaper(monitor, path.get<std::string>());
      }
    }
  }
}

void WallpaperManager::writeHyprpaperConfig() {
  // Get config path
  const char *configHome = std::getenv("XDG_CONFIG_HOME");
  std::string base;

  if (configHome) {
    base = configHome;
  } else {
    const char *home = std::getenv("HOME");
    base = std::string(home ? home : "") + "/.config";
  }

  std::string hyprpaperPath = base + "/hypr/hyprpaper.conf";

  std::ofstream file(hyprpaperPath);
  if (!file) {
    LOG_WARN("Could not write hyprpaper.conf: " + hyprpaperPath);
    return;
  }

  file << "# Auto-generated by BetterWallpaper\n";
  file << "# This file allows wallpaper to persist without the app running\n\n";
  file << "splash = false\n";
  file << "ipc = off\n\n";

  std::lock_guard<std::mutex> lock(m_mutex);

  // Preload all wallpapers
  for (const auto &[name, state] : m_monitors) {
    if (!state.currentPath.empty()) {
      file << "preload = " << state.currentPath << "\n";
    }
  }

  file << "\n";

  // Set wallpaper per monitor
  for (const auto &[name, state] : m_monitors) {
    if (!state.currentPath.empty()) {
      file << "wallpaper = " << name << "," << state.currentPath << "\n";
    }
  }

  file.close();
  LOG_INFO("Wrote hyprpaper.conf for native persistence: " + hyprpaperPath);
}

void WallpaperManager::startResourceMonitor() {
  if (m_monitorThread.joinable())
    return;

  m_stopMonitor = false;
  m_monitorThread = std::thread(&WallpaperManager::resourceMonitorLoop, this);
}

void WallpaperManager::stopResourceMonitor() {
  m_stopMonitor = true;
  if (m_monitorThread.joinable()) {
    m_monitorThread.join();
  }
}

void WallpaperManager::resourceMonitorLoop() {
  while (!m_stopMonitor) {
    // Check every 5 seconds
    for (int i = 0; i < 50 && !m_stopMonitor; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (m_stopMonitor)
      break;

    uint64_t rss = bwp::utils::SystemUtils::getProcessRSS();
    // Default 2GB limit if not set
    uint64_t limitMB = bwp::config::ConfigManager::getInstance().get<uint64_t>(
        "performance.ram_limit_mb", 2048);
    uint64_t limitBytes = limitMB * 1024 * 1024;

    if (rss > limitBytes) {
      LOG_WARN("High memory usage detected: " +
               bwp::utils::SystemUtils::formatBytes(rss) + " > " +
               std::to_string(limitMB) + "MB");

      // Send system notification
      bwp::core::NotificationManager::getInstance().sendSystemNotification(
          "Wallpaper Paused",
          "High memory usage detected (" +
              bwp::utils::SystemUtils::formatBytes(rss) +
              "). Wallpaper has been paused to save resources.",
          bwp::core::NotificationType::Warning);

      fallbackToStatic();

      // Wait longer after fallback to allow cleanup
      for (int i = 0; i < 300 && !m_stopMonitor; i++) { // 30s
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }
}

void WallpaperManager::fallbackToStatic() {
  bool triggered = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &[name, state] : m_monitors) {
      if (state.renderer) {
        auto type = state.renderer->getType();
        // Only stop heavy renderers
        if (type == WallpaperType::Video || type == WallpaperType::WEScene ||
            type == WallpaperType::WEVideo || type == WallpaperType::WEWeb) {

          LOG_INFO("Stopping heavy renderer on " + name + " due to RAM limit");
          state.renderer->stop();

          // Try to load static fallback (thumbnail)
          // Logic: look for preview.jpg/png in parent dir
          std::filesystem::path path(state.currentPath);
          std::filesystem::path dir = path.parent_path();
          std::string thumb;

          if (std::filesystem::exists(dir / "preview.jpg"))
            thumb = (dir / "preview.jpg").string();
          else if (std::filesystem::exists(dir / "preview.png"))
            thumb = (dir / "preview.png").string();
          else if (std::filesystem::exists(dir / "preview.gif"))
            thumb = (dir / "preview.gif").string();
          else
            thumb =
                state.currentPath; // Try itself if image (but it was heavy?)

          auto newRenderer = std::make_shared<StaticRenderer>();
          // If thumb fails, StaticRenderer handles it gracefully (black)
          // Or keeps old one? No, we stopped old one.

          if (newRenderer->load(thumb)) {
            state.renderer = newRenderer;
            state.window->transitionTo(newRenderer);
            newRenderer->render(nullptr, 0, 0); // Trigger?
            // Actually transitionTo handles render loop update
          }
          triggered = true;
        }
      }
    }
  }

  if (triggered) {
    // Notification
    LOG_WARN("Wallpapers switched to static due to high RAM usage.");
    bwp::core::NotificationManager::getInstance().sendSystemNotification(
        "High RAM Usage",
        "Wallpapers switched to static placeholders to save memory.",
        bwp::core::NotificationType::Warning);
  }
}

} // namespace bwp::wallpaper
