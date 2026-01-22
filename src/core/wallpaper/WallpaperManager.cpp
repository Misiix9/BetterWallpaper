#include "WallpaperManager.hpp"
#include "../hyprland/HyprlandManager.hpp"
#include "../monitor/MonitorManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "renderers/StaticRenderer.hpp"
#include "renderers/VideoRenderer.hpp"
#include "renderers/WallpaperEngineRenderer.hpp"

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
  std::lock_guard<std::mutex> lock(m_mutex);

  // Find monitor state
  auto it = m_monitors.find(monitorName);
  if (it == m_monitors.end()) {
    LOG_ERROR("Monitor not found: " + monitorName);
    return false;
  }

  // Determine type and create renderer
  auto renderer = createRenderer(path);
  if (!renderer) {
    LOG_ERROR("Failed to create renderer for path: " + path);
    return false;
  }

  if (!renderer->load(path)) {
    LOG_ERROR("Failed to load wallpaper: " + path);
    return false;
  }

  // Stop old renderer
  if (it->second.renderer) {
    it->second.renderer->stop();
  }

  it->second.renderer = renderer;
  it->second.currentPath = path;
  it->second.window->transitionTo(renderer);

  if (m_paused) {
    renderer->pause();
  } else {
    renderer->play();
  }

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

std::string
WallpaperManager::getCurrentWallpaper(const std::string &monitorName) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end()) {
    return it->second.currentPath;
  }
  return "";
}

} // namespace bwp::wallpaper
