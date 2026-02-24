#include "WallpaperManager.hpp"
#include "../config/ConfigManager.hpp"
#include "../hyprland/HyprlandManager.hpp"
#include "../monitor/MonitorManager.hpp"
#include "../notification/NotificationManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SafeProcess.hpp"
#include "../utils/SystemUtils.hpp"
#include "NativeWallpaperSetter.hpp"
#include "WallpaperPreloader.hpp"
#include "TransitionPolicy.hpp"
#include "renderers/StaticRenderer.hpp"
#include "renderers/VideoRenderer.hpp"
#include "renderers/WallpaperEngineRenderer.hpp"
#ifdef _WIN32
#include "WindowsWallpaperSetter.hpp"
#endif
#include <cstdlib>
#include <fstream>
#include <glib.h>
#include <vector>
namespace bwp::wallpaper {
namespace {

// keep recent wallpapers in config, max 20 seems plenty for now
void updateHistory(const std::string &path) {
  auto &conf = bwp::config::ConfigManager::getInstance();
  std::vector<std::string> history;
  try {
    history = conf.get<std::vector<std::string>>("wallpaper_history", {});
  } catch (...) {
  }
  auto it = std::remove(history.begin(), history.end(), path);
  history.erase(it, history.end());
  history.insert(history.begin(), path);
  if (history.size() > 20) {
    history.resize(20);
  }
  conf.set("wallpaper_history", history);
}
} // namespace

WallpaperManager &WallpaperManager::getInstance() {
  static WallpaperManager instance;
  return instance;
}
WallpaperManager::WallpaperManager() {}
WallpaperManager::~WallpaperManager() { stopResourceMonitor(); }

void WallpaperManager::initialize() {
  // Kill ALL leftover linux-wallpaperengine processes from previous sessions.
  // When the daemon stops, WE processes survive (setsid + persistence).
  // Without this cleanup, old processes compete with new ones for the
  // same Wayland layer surface, causing "random" wallpaper failures.
  WallpaperEngineRenderer::killAllExistingProcesses();

  monitor::MonitorManager::getInstance().setCallback(
      [this](const monitor::MonitorInfo &info, bool connected) {
        this->handleMonitorUpdate(info, connected);
      });
  monitor::MonitorManager::getInstance().initialize();
  auto monitors = monitor::MonitorManager::getInstance().getMonitors();
  for (const auto &m : monitors) {
    handleMonitorUpdate(m, true);
  }
#ifndef _WIN32
  ::bwp::hyprland::HyprlandManager::getInstance().initialize();
#endif
  startResourceMonitor();
}

void WallpaperManager::handleMonitorUpdate(const monitor::MonitorInfo &info,
                                           bool connected) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
#ifndef _WIN32
  // on Linux we need to spawn a GTK window for gtk4-layer-shell
  if (connected) {
    if (info.name.empty()) {
      LOG_WARN("Skipping monitor with empty name (ID: " +
               std::to_string(info.id) + ")");
      return;
    }
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
#else
  // Windows handles the windowing, we just track state
  if (connected) {
    if (m_monitors.find(info.name) == m_monitors.end()) {
      m_monitors[info.name] = MonitorState();
    }
  }
#endif
}

bool WallpaperManager::setWallpaper(const std::string &monitorName,
                                    const std::string &path) {
  // Log this because users always complain about black screens if path is wrong
  LOG_INFO("WallpaperManager::setWallpaper called for " + monitorName +
           " with path: " + path);
#ifdef _WIN32
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_monitors.find(monitorName) == m_monitors.end()) {
    m_monitors[monitorName] = MonitorState();
  }
  m_monitors[monitorName].currentPath = path;
  WindowsWallpaperSetter setter;
  bool result = setter.setWallpaper(path, monitorName);
  if (result) {
    LOG_INFO("Windows wallpaper set successfully.");
    updateHistory(path);
    saveState();
  } else {
    LOG_ERROR("Failed to set Windows wallpaper.");
    updateHistory(path);
    saveState();
  }
  return result;
#else
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  LOG_INFO("Currently have " + std::to_string(m_monitors.size()) + " monitors");
  {
    auto existing = m_monitors.find(monitorName);
    if (existing != m_monitors.end() && existing->second.currentPath == path &&
        existing->second.renderer && existing->second.renderer->isPlaying() &&
        !path.empty()) {
      LOG_INFO("Wallpaper already set to " + path + " on " + monitorName +
               " — skipping re-application");
      return true;
    }
  }
  auto it = m_monitors.find(monitorName);
  if (it == m_monitors.end()) {
    LOG_WARN("Monitor not found: " + monitorName +
             ". Activating Blind Mode fallback.");
    MonitorState state;
    monitor::MonitorInfo info;
    info.name = monitorName;
    try {
      state.window = std::make_shared<WallpaperWindow>(info);
      state.window->show();
    } catch (...) {
      LOG_ERROR("Failed to create Blind Mode window for " + monitorName);
    }
    m_monitors[monitorName] = state;
    it = m_monitors.find(monitorName);
  }
  LOG_INFO("Found monitor " + monitorName);
  {
    auto &conf = bwp::config::ConfigManager::getInstance();
    // Prefer per-wallpaper resolved settings (written by PreviewPanel before
    // IPC), falling back to global defaults
    int perWpFps = conf.get<int>("wallpapers.current_settings.fps_limit", -999);
    if (perWpFps != -999) {
      m_fpsLimit = perWpFps;
      m_volumeLevel =
          conf.get<int>("wallpapers.current_settings.volume", m_volumeLevel);
      m_globalMuted =
          conf.get<bool>("wallpapers.current_settings.muted", m_globalMuted);
      m_noAudioProcessing =
          conf.get<bool>("wallpapers.current_settings.no_audio_processing",
                         m_noAudioProcessing);
      m_disableMouse = conf.get<bool>(
          "wallpapers.current_settings.disable_mouse", m_disableMouse);
      m_noAutomute = conf.get<bool>("wallpapers.current_settings.no_automute",
                                    m_noAutomute);
    } else {
      m_fpsLimit = conf.get<int>("performance.fps_limit", m_fpsLimit);
      m_volumeLevel = conf.get<int>("defaults.audio_volume", m_volumeLevel);
      bool audioEnabled =
          conf.get<bool>("defaults.audio_enabled", !m_globalMuted);
      m_globalMuted = !audioEnabled;
      m_noAudioProcessing =
          conf.get<bool>("defaults.no_audio_processing", m_noAudioProcessing);
      m_disableMouse = conf.get<bool>("defaults.disable_mouse", m_disableMouse);
      m_noAutomute = conf.get<bool>("defaults.no_automute", m_noAutomute);
    }
  }
  // Disable auto-restart on the old renderer BEFORE launching the new one.
  // Without this, killExistingProcessesForMonitors kills the old process,
  // the old watcher detects the "crash" and respawns it, creating a zombie.
  if (it->second.renderer) {
    it->second.renderer->prepareForReplacement();
  }

  std::shared_ptr<WallpaperRenderer> renderer;
  auto &preloader = WallpaperPreloader::getInstance();
  if (preloader.isReady(path)) {
    LOG_INFO("Using preloaded renderer for instant wallpaper setting: " + path);
    renderer = preloader.getPreloadedRenderer(path);
  }
  if (!renderer) {
    renderer = createRenderer(path);
    if (!renderer) {
      LOG_ERROR("Failed to create renderer for path: " + path);
      return false;
    }
    LOG_INFO("Created renderer for wallpaper");
    renderer->setMonitor(monitorName);
    auto weRenderer =
        std::dynamic_pointer_cast<WallpaperEngineRenderer>(renderer);
    if (weRenderer) {
      weRenderer->setFpsLimit(m_fpsLimit);
      weRenderer->setMuted(m_globalMuted);
      weRenderer->setNoAudioProcessing(m_noAudioProcessing);
      weRenderer->setDisableMouse(m_disableMouse);
      weRenderer->setNoAutomute(m_noAutomute);
      weRenderer->setVolumeLevel(m_volumeLevel);
    }
    if (!renderer->load(path)) {
      LOG_ERROR("Failed to load wallpaper: " + path);
      return false;
    }
  } else {
    renderer->setMonitor(monitorName);
    auto weRenderer =
        std::dynamic_pointer_cast<WallpaperEngineRenderer>(renderer);
    if (weRenderer) {
      LOG_INFO("WE renderer detected - calling load() to set path and spawn "
               "process");
      if (!renderer->load(path)) {
        LOG_ERROR("Failed to load WE wallpaper: " + path);
        return false;
      }
    } else {
      LOG_INFO("Preloaded renderer ready, skipping load step (instant!)");
    }
  }
  LOG_INFO("Loaded wallpaper successfully");
  std::shared_ptr<WallpaperRenderer> oldRenderer = it->second.renderer;
  auto &monitorManager = monitor::MonitorManager::getInstance();
  auto monitors = monitorManager.getMonitors();
  monitor::MonitorInfo monitorInfo;
  bool foundMonitor = false;
  for (const auto &mon : monitors) {
    if (mon.name == monitorName) {
      monitorInfo = mon;
      foundMonitor = true;
      break;
    }
  }
  if (!foundMonitor) {
    LOG_WARN("Monitor info not found for: " + monitorName +
             ", using basic info");
    monitorInfo.name = monitorName;
  }
  if (!it->second.window) {
    it->second.window = std::make_shared<WallpaperWindow>(monitorInfo);
    it->second.window->show();
  }
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto policy = computeTransitionPolicy(conf);
  auto plan = makeTransitionPlan(policy);

  if (m_scalingModes.count(monitorName)) {
    renderer->setScalingMode(
        static_cast<ScalingMode>(m_scalingModes[monitorName]));
  }

  // Choose transition path
  auto isNextWE = std::dynamic_pointer_cast<WallpaperEngineRenderer>(renderer) != nullptr;

  if (plan.useWindowTransition && it->second.renderer) {
    it->second.window->transitionTo(renderer, [this, monitorName, oldRenderer, plan]() {
      if (oldRenderer) {
        auto *captured = new std::shared_ptr<WallpaperRenderer>(oldRenderer);
        int delay = plan.stopOldRendererDelayMs;
        if (delay <= 0) {
          (*captured)->stop();
          delete captured;
        } else {
          g_timeout_add(
              delay,
              +[](gpointer data) -> gboolean {
                auto *r = static_cast<std::shared_ptr<WallpaperRenderer> *>(data);
                (*r)->stop();
                delete r;
                return G_SOURCE_REMOVE;
              },
              captured);
        }
      }
      LOG_INFO("Seamless transition completed for monitor: " + monitorName);
    });
  } else {
    // Direct swap without animation
    if (isNextWE) {
      // Do not bind WE renderer to our surface (it renders transparent). Start it and hide our window to reveal external surface.
      renderer->play();
      it->second.window->hide();
      if (oldRenderer) {
        auto *captured = new std::shared_ptr<WallpaperRenderer>(oldRenderer);
        int delay = plan.stopOldRendererDelayMs;
        if (delay <= 0) {
          (*captured)->stop();
          delete captured;
        } else {
          g_timeout_add(
              delay,
              +[](gpointer data) -> gboolean {
                auto *r = static_cast<std::shared_ptr<WallpaperRenderer> *>(data);
                (*r)->stop();
                delete r;
                return G_SOURCE_REMOVE;
              },
              captured);
        }
      }
    } else {
      it->second.window->setRenderer(renderer);
      it->second.window->show();
      if (oldRenderer) {
        oldRenderer->stop();
      }
    }
  }

  it->second.renderer = renderer;
  it->second.currentPath = path;

  if (m_paused) {
    renderer->pause();
  }

  LOG_INFO("Wallpaper set initiated (Window transition path)");
  updateHistory(path);
  saveState();
  writeHyprpaperConfig();
  return true;
#endif
}
bool WallpaperManager::setWallpaper(const std::vector<std::string> &monitors,
                                    const std::string &path) {
  if (monitors.empty())
    return false;
  if (monitors.size() == 1)
    return setWallpaper(monitors[0], path);
  std::string mime = utils::FileUtils::getMimeType(path);
  bool isWE = false;
  if (mime.find("x-wallpaper-engine") != std::string::npos)
    isWE = true;
  else {
    std::string ext = utils::FileUtils::getExtension(path);
    if (ext == "pkg" || ext == "json")
      isWE = true;
  }
  if (!isWE) {
    bool allSuccess = true;
    for (const auto &mon : monitors) {
      if (!setWallpaper(mon, path))
        allSuccess = false;
    }
    return allSuccess;
  }
  LOG_INFO("Setting shared wallpaper for " + std::to_string(monitors.size()) +
           " monitors: " + path);
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<std::string> validMonitors;
  for (const auto &name : monitors) {
    if (m_monitors.find(name) != m_monitors.end()) {
      validMonitors.push_back(name);
    } else {
      LOG_WARN("Monitor not found during bulk set: " + name);
    }
  }
  if (validMonitors.empty()) {
    LOG_WARN("No valid monitors found in detection list for bulk set. Falling "
             "back to requested monitors (Blind Mode).");
    validMonitors = monitors;
    for (const auto &name : validMonitors) {
      if (m_monitors.find(name) == m_monitors.end()) {
        MonitorState state;
        monitor::MonitorInfo info;
        info.name = name;
        try {
          state.window = std::make_shared<WallpaperWindow>(info);
          state.window->show();
        } catch (...) {
          LOG_ERROR("Failed to create Blind Mode window for " + name);
        }
        m_monitors[name] = state;
      }
    }
  }
  // Disable auto-restart on all old renderers before launching new shared one
  for (const auto &name : validMonitors) {
    auto &state = m_monitors[name];
    if (state.renderer) {
      state.renderer->prepareForReplacement();
    }
  }

  auto renderer = createRenderer(path);
  if (!renderer) {
    LOG_ERROR("Failed to create shared renderer for path: " + path);
    return false;
  }
  auto weRenderer =
      std::dynamic_pointer_cast<WallpaperEngineRenderer>(renderer);
  if (weRenderer) {
    weRenderer->setMonitors(validMonitors);
    weRenderer->setFpsLimit(m_fpsLimit);
    weRenderer->setMuted(m_globalMuted);
    weRenderer->setNoAudioProcessing(m_noAudioProcessing);
    weRenderer->setDisableMouse(m_disableMouse);
    weRenderer->setNoAutomute(m_noAutomute);
    weRenderer->setVolumeLevel(m_volumeLevel);
  }
  if (!renderer->load(path)) {
    LOG_ERROR("Failed to load shared wallpaper: " + path);
    return false;
  }
  for (const auto &name : validMonitors) {
    auto &state = m_monitors[name];
    auto oldRenderer = state.renderer;
    state.renderer = renderer;
    state.currentPath = path;
    state.window->transitionTo(renderer);
    if (m_scalingModes.count(name)) {
      renderer->setScalingMode(static_cast<ScalingMode>(m_scalingModes[name]));
    }
    if (oldRenderer) {
      // Stop old renderer after transition delay via main-loop timer
      // (avoids unjoined detached threads)
      auto *captured = new std::shared_ptr<WallpaperRenderer>(oldRenderer);
      g_timeout_add(
          800,
          +[](gpointer data) -> gboolean {
            auto *r = static_cast<std::shared_ptr<WallpaperRenderer> *>(data);
            (*r)->stop();
            delete r;
            return G_SOURCE_REMOVE;
          },
          captured);
    }
  }
  if (m_paused) {
    renderer->pause();
  }
  LOG_INFO("Shared wallpaper set successfully!");
  updateHistory(path);
  saveState();
  writeHyprpaperConfig();
  return true;
}
std::shared_ptr<WallpaperRenderer>
WallpaperManager::createRenderer(const std::string &path) {
  std::string mime = utils::FileUtils::getMimeType(path);
  if (mime.find("image/") != std::string::npos &&
      mime.find("gif") == std::string::npos) {
    return std::make_shared<StaticRenderer>();
  } else if (mime.find("video/") != std::string::npos ||
             mime.find("gif") != std::string::npos) {
    return std::make_shared<VideoRenderer>();
  } else if (mime.find("x-wallpaper-engine") != std::string::npos) {
    return std::make_shared<WallpaperEngineRenderer>();
  }
  std::string ext = utils::FileUtils::getExtension(path);
  if (ext == "mp4" || ext == "webm" || ext == "mkv" || ext == "gif") {
    return std::make_shared<VideoRenderer>();
  }
  if (ext == "pkg" || ext == "json" || ext == "html" || ext == "htm") {
    return std::make_shared<WallpaperEngineRenderer>();
  }
  return std::make_shared<StaticRenderer>();
}
void WallpaperManager::setPaused(bool paused) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_globalMuted = muted;
  for (auto &pair : m_monitors) {
    if (pair.second.renderer) {
      pair.second.renderer->setMuted(muted);
    }
  }
}
void WallpaperManager::setMuted(const std::string &monitorName, bool muted) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_monitors.find(monitorName) != m_monitors.end()) {
    if (m_monitors[monitorName].renderer) {
      m_monitors[monitorName].renderer->setMuted(muted);
    }
  }
}
void WallpaperManager::setVolume(const std::string &monitorName, int volume) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_monitors.find(monitorName) != m_monitors.end()) {
    if (m_monitors[monitorName].renderer) {
      m_monitors[monitorName].renderer->setVolume(static_cast<float>(volume) /
                                                  100.0f);
    }
  }
}
void WallpaperManager::pause(const std::string &monitorName) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end() && it->second.renderer) {
    it->second.renderer->pause();
  }
}
void WallpaperManager::resume(const std::string &monitorName) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end() && it->second.renderer) {
    it->second.renderer->play();
  }
}
void WallpaperManager::stop(const std::string &monitorName) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end() && it->second.renderer) {
    it->second.renderer->stop();
  }
}
std::string
WallpaperManager::getCurrentWallpaper(const std::string &monitorName) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end()) {
    return it->second.currentPath;
  }
  return "";
}

bool WallpaperManager::isPaused() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_paused;
}

bool WallpaperManager::isMuted() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_globalMuted;
}

int WallpaperManager::getVolume() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_volumeLevel;
}

void WallpaperManager::killConflictingWallpapers() {
  const std::vector<std::string> conflicts = {
      "hyprpaper",  "swaybg",   "swww-daemon", "mpvpaper",    "wpaperd",
      "waypaper",   "aztime",   "glpaper",     "feh",         "nitrogen",
      "xwallpaper", "hsetroot", "habak",       "displayball", "eww-daemon",
      "ags",        "swww"};
  for (const auto &proc : conflicts) {
    bwp::utils::SafeProcess::exec({"pkill", "-9", "-x", proc});
  }
}
void WallpaperManager::saveState() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  nlohmann::json wallpapers;
  for (const auto &[name, state] : m_monitors) {
    if (!state.currentPath.empty() &&
        state.currentPath != "TRANSITION_FREEZE") {
      wallpapers[name] = state.currentPath;
    }
  }
  conf.set("wallpapers.current", wallpapers);
  conf.save();
  LOG_INFO("Saved wallpaper state for " + std::to_string(wallpapers.size()) +
           " monitors to disk");
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
void WallpaperManager::shutdown() {
  LOG_INFO("Shutting down WallpaperManager...");
  stopResourceMonitor();
  auto &config = config::ConfigManager::getInstance();
  bool persistence = config.get<bool>("wallpaper.persistence_on_close", true);
  LOG_INFO("Wallpaper persistence on close: " +
           std::string(persistence ? "enabled" : "disabled"));
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      if (persistence) {
        auto weRenderer =
            std::dynamic_pointer_cast<WallpaperEngineRenderer>(state.renderer);
        if (weRenderer) {
          weRenderer->detach();
        } else {
          state.renderer->stop();
        }
      } else {
        state.renderer->stop();
      }
    }
    if (state.window) {
      state.window->hide();
    }
  }
  m_monitors.clear();
}
void WallpaperManager::writeHyprpaperConfig() {
#ifndef _WIN32
  const char *configHome = std::getenv("XDG_CONFIG_HOME");
  std::string base;
  if (configHome) {
    base = configHome;
  } else {
    const char *home = std::getenv("HOME");
    if (!home) {
      LOG_ERROR(
          "writeHyprpaperConfig: neither XDG_CONFIG_HOME nor HOME is set");
      return;
    }
    base = std::string(home) + "/.config";
  }
  std::string hyprpaperPath = base + "/hypr/hyprpaper.conf";
  std::ofstream file(hyprpaperPath);
  if (!file) {
    LOG_WARN("Could not write hyprpaper.conf: " + hyprpaperPath);
    return;
  }
  file << "# Auto-generated by BetterWallpaper\n";
  file << "# This file allows wallpaper to persist without the app "
          "running\n\n";
  file << "splash = false\n";
  file << "ipc = off\n\n";
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  for (const auto &[name, state] : m_monitors) {
    if (!state.currentPath.empty()) {
      file << "preload = " << state.currentPath << "\n";
    }
  }
  file << "\n";
  for (const auto &[name, state] : m_monitors) {
    if (!state.currentPath.empty()) {
      file << "wallpaper = " << name << "," << state.currentPath << "\n";
    }
  }
  file.close();
  LOG_INFO("Wrote hyprpaper.conf for native persistence: " + hyprpaperPath);
#endif
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
    for (int i = 0; i < 50 && !m_stopMonitor; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (m_stopMonitor)
      break;
    uint64_t rss = bwp::utils::SystemUtils::getProcessRSS();
    uint64_t limitMB = bwp::config::ConfigManager::getInstance().get<uint64_t>(
        "performance.ram_limit_mb", 2048);
    uint64_t limitBytes = limitMB * 1024 * 1024;
    if (rss > limitBytes) {
      LOG_WARN("High memory usage detected: " +
               bwp::utils::SystemUtils::formatBytes(rss) + " > " +
               std::to_string(limitMB) + "MB");
      bwp::core::NotificationManager::getInstance().sendSystemNotification(
          "Wallpaper Paused",
          "High memory usage detected (" +
              bwp::utils::SystemUtils::formatBytes(rss) +
              "). Wallpaper has been paused to save resources.",
          bwp::core::NotificationType::Warning);
      fallbackToStatic();
      for (int i = 0; i < 300 && !m_stopMonitor; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }
}
void WallpaperManager::fallbackToStatic() {
  bool triggered = false;
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto &[name, state] : m_monitors) {
      if (state.renderer) {
        auto type = state.renderer->getType();
        if (type == WallpaperType::Video || type == WallpaperType::WEScene ||
            type == WallpaperType::WEVideo || type == WallpaperType::WEWeb) {
          LOG_INFO("Stopping heavy renderer on " + name + " due to RAM limit");
          state.renderer->stop();
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
            thumb = state.currentPath;
          auto newRenderer = std::make_shared<StaticRenderer>();
          if (newRenderer->load(thumb)) {
            state.renderer = newRenderer;
            state.window->transitionTo(newRenderer);
            newRenderer->render(nullptr, 0, 0);
          }
          triggered = true;
        }
      }
    }
  }
  if (triggered) {
    LOG_WARN("Wallpapers switched to static due to high RAM usage.");
    bwp::core::NotificationManager::getInstance().sendSystemNotification(
        "High RAM Usage",
        "Wallpapers switched to static placeholders to save memory.",
        bwp::core::NotificationType::Warning);
  }
}
void WallpaperManager::setScalingMode(const std::string &monitorName,
                                      int mode) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_scalingModes[monitorName] = mode;
  auto it = m_monitors.find(monitorName);
  if (it != m_monitors.end() && it->second.renderer) {
    it->second.renderer->setScalingMode(static_cast<ScalingMode>(mode));
  }
}
void WallpaperManager::setFpsLimit(int fps) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_fpsLimit = fps;
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      auto weRenderer =
          std::dynamic_pointer_cast<WallpaperEngineRenderer>(state.renderer);
      if (weRenderer) {
        weRenderer->setFpsLimit(fps);
      }
    }
  }
}
void WallpaperManager::setNoAudioProcessing(bool enabled) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_noAudioProcessing = enabled;
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      auto weRenderer =
          std::dynamic_pointer_cast<WallpaperEngineRenderer>(state.renderer);
      if (weRenderer) {
        weRenderer->setNoAudioProcessing(enabled);
      }
    }
  }
}
void WallpaperManager::setDisableMouse(bool enabled) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_disableMouse = enabled;
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      auto weRenderer =
          std::dynamic_pointer_cast<WallpaperEngineRenderer>(state.renderer);
      if (weRenderer) {
        weRenderer->setDisableMouse(enabled);
      }
    }
  }
}
void WallpaperManager::setNoAutomute(bool enabled) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_noAutomute = enabled;
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      auto weRenderer =
          std::dynamic_pointer_cast<WallpaperEngineRenderer>(state.renderer);
      if (weRenderer) {
        weRenderer->setNoAutomute(enabled);
      }
    }
  }
}
void WallpaperManager::setVolumeLevel(int volume) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_volumeLevel = volume;
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      auto weRenderer =
          std::dynamic_pointer_cast<WallpaperEngineRenderer>(state.renderer);
      if (weRenderer) {
        weRenderer->setVolumeLevel(volume);
      }
    }
  }
}
} // namespace bwp::wallpaper
