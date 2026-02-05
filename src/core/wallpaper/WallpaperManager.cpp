#include "WallpaperManager.hpp"
#include "../config/ConfigManager.hpp"
#include "../hyprland/HyprlandManager.hpp"
#include "../monitor/MonitorManager.hpp"
#include "../notification/NotificationManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SystemUtils.hpp"
#include "NativeWallpaperSetter.hpp"
#include "WallpaperPreloader.hpp"
#include "WallpaperTransitionManager.hpp"
#include "renderers/StaticRenderer.hpp"
#include "renderers/VideoRenderer.hpp"
#include "renderers/WallpaperEngineRenderer.hpp"
#ifdef _WIN32
#include "WindowsWallpaperSetter.hpp"
#endif
#include <cstdlib>
#include <fstream>
#include <vector>

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

#ifndef _WIN32
  // Initialize Hyprland integration
  ::bwp::hyprland::HyprlandManager::getInstance().initialize();
#endif
  // Start Resource Monitor
  startResourceMonitor();
}

void WallpaperManager::handleMonitorUpdate(const monitor::MonitorInfo &info,
                                           bool connected) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
#ifndef _WIN32
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
#else
  // Windows: Keep track of monitor presence but don't create custom windows
  if (connected) {
    if (m_monitors.find(info.name) == m_monitors.end()) {
      m_monitors[info.name] = MonitorState();
    }
  }
#endif
}

// #region agent log
static void debugLog(const std::string &loc, const std::string &msg,
                     const std::string &hyp) {
  FILE *f = fopen(
      "/home/onxy/Documents/scripts/BetterWallpaper/.cursor/debug.log", "a");
  if (f) {
    fprintf(f,
            "{\"location\":\"%s\",\"message\":\"%s\",\"hypothesisId\":\"%s\","
            "\"timestamp\":%ld}\n",
            loc.c_str(), msg.c_str(), hyp.c_str(), (long)time(nullptr));
    fflush(f);
    fclose(f);
  }
}
// #endregion

bool WallpaperManager::setWallpaper(const std::string &monitorName,
                                    const std::string &path) {
  // #region agent log
  debugLog("WallpaperManager.cpp:setWallpaper:entry", "setWallpaper called",
           "A");
  // #endregion
  LOG_INFO("WallpaperManager::setWallpaper called for " + monitorName +
           " with path: " + path);

#ifdef _WIN32
  // Windows Implementation
  // Bypassing renderers and windows for now
  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  // Track metadata
  if (m_monitors.find(monitorName) == m_monitors.end()) {
    m_monitors[monitorName] = MonitorState();
  }
  m_monitors[monitorName].currentPath = path;

  WindowsWallpaperSetter setter;
  bool result =
      setter.setWallpaper(path, monitorName); // Assuming method call valid
  if (result) {
    LOG_INFO("Windows wallpaper set successfully.");
    saveState();
  } else {
    LOG_ERROR("Failed to set Windows wallpaper.");
  }
  return result;
#else

  LOG_INFO("WallpaperManager::setWallpaper called for " + monitorName +
           " with path: " + path);

  // #region agent log
  debugLog("WallpaperManager.cpp:mutex", "About to acquire mutex", "E");
  // #endregion
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  // #region agent log
  debugLog("WallpaperManager.cpp:mutex_acquired", "Mutex acquired", "E");
  // #endregion

  LOG_INFO("Currently have " + std::to_string(m_monitors.size()) + " monitors");

  // Find monitor state
  auto it = m_monitors.find(monitorName);
  if (it == m_monitors.end()) {
    LOG_WARN("Monitor not found: " + monitorName +
             ". Activating Blind Mode fallback.");
    // Create blind state
    MonitorState state;
    monitor::MonitorInfo info;
    info.name = monitorName;
    // Attempt to create window even if blind - might be needed for static/video
    // fallbacks If it fails, we catch it? WallpaperWindow constructor usually
    // safe? Let's assume yes.
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

  // #region agent log
  debugLog("WallpaperManager.cpp:createRenderer", "About to createRenderer",
           "C");
  // #endregion

  // Check if wallpaper is already preloaded for instant setting
  std::shared_ptr<WallpaperRenderer> renderer;
  auto &preloader = WallpaperPreloader::getInstance();

  if (preloader.isReady(path)) {
    LOG_INFO("Using preloaded renderer for instant wallpaper setting: " + path);
    renderer = preloader.getPreloadedRenderer(path);
  }

  // If not preloaded, create renderer the normal way
  if (!renderer) {
    renderer = createRenderer(path);
    // #region agent log
    debugLog("WallpaperManager.cpp:createRenderer_done", "createRenderer done",
             "C");
    // #endregion
    if (!renderer) {
      LOG_ERROR("Failed to create renderer for path: " + path);
      return false;
    }

    LOG_INFO("Created renderer for wallpaper");

    renderer->setMonitor(monitorName);

    // #region agent log
    debugLog("WallpaperManager.cpp:load", "About to load renderer", "C");
    // #endregion
    if (!renderer->load(path)) {
      LOG_ERROR("Failed to load wallpaper: " + path);
      return false;
    }
    // #region agent log
    debugLog("WallpaperManager.cpp:load_done", "Renderer load done", "C");
    // #endregion
  } else {
    // Preloaded renderer - set monitor
    renderer->setMonitor(monitorName);

    // IMPORTANT: For WallpaperEngineRenderer, we MUST call load() even if preloaded
    // because load() sets m_pkPath and triggers the process spawn.
    // The "preload" for WE only validates assets, it doesn't actually preload.
    auto weRenderer =
        std::dynamic_pointer_cast<WallpaperEngineRenderer>(renderer);
    if (weRenderer) {
      LOG_INFO("WE renderer detected - calling load() to set path and spawn process");
      if (!renderer->load(path)) {
        LOG_ERROR("Failed to load WE wallpaper: " + path);
        return false;
      }
    } else {
      LOG_INFO("Preloaded renderer ready, skipping load step (instant!)");
    }
  }

  LOG_INFO("Loaded wallpaper successfully");

  // === SEAMLESS WALLPAPER SWITCHING ===
  // Key principle: New wallpaper appears IN FRONT of old wallpaper
  // 1. Old wallpaper stays visible and running
  // 2. Create NEW window for new wallpaper IN FRONT (opacity 0 initially)
  // 3. Fade new wallpaper in (opacity 0 → 1)
  // 4. After transition completes, destroy old window

  // Save reference to old window (if any)
  std::shared_ptr<WallpaperWindow> oldWindow = it->second.window;
  std::shared_ptr<WallpaperRenderer> oldRenderer = it->second.renderer;
  std::string oldPath = it->second.currentPath;

  // Get monitor info for creating new window
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
    // Fallback - use basic info
    LOG_WARN("Monitor info not found for: " + monitorName +
             ", using basic info");
    monitorInfo.name = monitorName;
  }

  // Create NEW window for the new wallpaper (will be in front of old one)
  auto newWindow = std::make_shared<WallpaperWindow>(monitorInfo);
  newWindow->setRenderer(renderer);
  newWindow->setOpacity(0.0); // Start invisible

  // Apply settings to new renderer
  if (m_paused) {
    renderer->pause();
  } else {
    renderer->play();
  }

  if (m_scalingModes.count(monitorName)) {
    renderer->setScalingMode(
        static_cast<ScalingMode>(m_scalingModes[monitorName]));
  }

  // Show new window (invisible but in front)
  newWindow->show();

  LOG_INFO("New wallpaper window created IN FRONT with opacity 0");

  // Use WallpaperTransitionManager for smooth transition
  auto &transitionManager = WallpaperTransitionManager::getInstance();
  transitionManager.loadSettings(); // Ensure latest settings

  // Store new state immediately (even before transition completes)
  it->second.window = newWindow;
  it->second.renderer = renderer;
  it->second.currentPath = path;

  // Start transition: new window fades in over old window
  transitionManager.startTransition(
      monitorName, oldWindow, newWindow,
      [this, monitorName, path, oldPath](bool success) {
        if (success) {
          LOG_INFO("Seamless transition completed for monitor: " + monitorName);
        } else {
          LOG_WARN("Transition failed for monitor: " + monitorName);
        }
        // Old window and renderer will be automatically cleaned up
        // when their shared_ptrs go out of scope
      });

  // Return true immediately, effectively "set" from user perspective
  LOG_INFO("Wallpaper set initiated (Transitioning)");
  saveState();
  writeHyprpaperConfig();
  return true;
#endif
}
bool WallpaperManager::setWallpaper(const std::vector<std::string> &monitors,
                                    const std::string &path) {
  // #region agent log
  debugLog("WallpaperManager.cpp:bulk_entry", "bulk setWallpaper called", "I");
  // #endregion

  if (monitors.empty())
    return false;
  if (monitors.size() == 1)
    return setWallpaper(monitors[0], path);

  // #region agent log
  debugLog("WallpaperManager.cpp:bulk_getMime", "About to call getMimeType",
           "I");
  // #endregion

  // Check if type supports multi-monitor single process
  std::string mime = utils::FileUtils::getMimeType(path);

  // #region agent log
  debugLog("WallpaperManager.cpp:bulk_afterMime",
           "getMimeType returned: " + mime, "I");
  // #endregion
  bool isWE = false;
  if (mime.find("x-wallpaper-engine") != std::string::npos)
    isWE = true;
  else {
    std::string ext = utils::FileUtils::getExtension(path);
    if (ext == "pkg" || ext == "json")
      isWE = true;
  }

  if (!isWE) {
    // #region agent log
    debugLog("WallpaperManager.cpp:bulk_notWE",
             "Not WE, falling back to per-monitor", "I");
    // #endregion
    // Fallback for types that don't support multi-monitor instances (Video,
    // Static)
    bool allSuccess = true;
    for (const auto &mon : monitors) {
      // #region agent log
      debugLog("WallpaperManager.cpp:bulk_loop",
               "Calling single setWallpaper for: " + mon, "I");
      // #endregion
      if (!setWallpaper(mon, path))
        allSuccess = false;
    }
    return allSuccess;
  }

  // #region agent log
  debugLog("WallpaperManager.cpp:bulk_isWE",
           "Is WE, using shared renderer path", "I");
  // #endregion

  // Optimized path for Wallpaper Engine
  LOG_INFO("Setting shared wallpaper for " + std::to_string(monitors.size()) +
           " monitors: " + path);

  // #region agent log
  debugLog("WallpaperManager.cpp:WE_mutex", "About to acquire mutex (WE path)",
           "J");
  // #endregion
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  // #region agent log
  debugLog("WallpaperManager.cpp:WE_mutex_acquired", "Mutex acquired (WE path)",
           "J");
  // #endregion

  // Validate monitors
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

    // Initialize state for blind monitors
    for (const auto &name : validMonitors) {
      if (m_monitors.find(name) == m_monitors.end()) {
        MonitorState state;
        monitor::MonitorInfo info;
        info.name = name;
        try {
          // Assuming WallpaperWindow can be created without strict validation
          state.window = std::make_shared<WallpaperWindow>(info);
          state.window->show();
        } catch (...) {
          LOG_ERROR("Failed to create Blind Mode window for " + name);
        }
        m_monitors[name] = state;
      }
    }
  }

  // #region agent log
  debugLog("WallpaperManager.cpp:WE_createRenderer", "About to createRenderer",
           "J");
  // #endregion
  auto renderer = createRenderer(path);
  // #region agent log
  debugLog("WallpaperManager.cpp:WE_createRenderer_done", "createRenderer done",
           "J");
  // #endregion
  if (!renderer)
    return false;

  // Setup renderer
  renderer->setMonitors(validMonitors);
  // #region agent log
  debugLog("WallpaperManager.cpp:WE_load", "About to call renderer->load", "J");
  // #endregion
  if (!renderer->load(path)) {
    LOG_ERROR("Failed to load shared wallpaper: " + path);
    return false;
  }
  // #region agent log
  debugLog("WallpaperManager.cpp:WE_load_done", "renderer->load done", "J");
  // #endregion

  // Assign to all monitors
  for (const auto &name : validMonitors) {
    // #region agent log
    debugLog("WallpaperManager.cpp:WE_loop", "Processing monitor: " + name,
             "J");
    // #endregion
    auto &state = m_monitors[name];

    // #region agent log
    debugLog("WallpaperManager.cpp:WE_loop", "Processing monitor: " + name,
             "J");
    // #endregion

    // Hold reference to old renderer to prevent immediate destruction
    auto oldRenderer = state.renderer;

    // Assign new renderer immediately so UI updates
    state.renderer = renderer;
    state.currentPath = path;

    // #region agent log
    debugLog("WallpaperManager.cpp:WE_transitionTo",
             "About to call transitionTo", "J");
    // #endregion

    // Note: WallpaperEngineRenderer::render() draws nothing (external window),
    // so transitionTo()'s snapshot will be empty/transparent.
    // The "transition" here is effectively managing state.
    state.window->transitionTo(renderer);

    // #region agent log
    debugLog("WallpaperManager.cpp:WE_transitionTo_done", "transitionTo done",
             "J");
    // #endregion

    if (m_scalingModes.count(name)) {
      renderer->setScalingMode(static_cast<ScalingMode>(m_scalingModes[name]));
    }

    // Stop the old renderer after a delay to allow the new process to appear
    if (oldRenderer) {
      // Shared pointer capture keeps it alive
      std::thread([oldRenderer]() {
#ifdef _WIN32
        Sleep(800); // 800ms delay
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
#endif
        oldRenderer->stop();
      }).detach();
    }
  }

  // #region agent log
  debugLog("WallpaperManager.cpp:WE_before_play", "About to call play/pause",
           "K");
  // #endregion
  if (m_paused) {
    renderer->pause();
  } else {
    LOG_INFO("Starting shared wallpaper playback");
    renderer->play();
  }
  // #region agent log
  debugLog("WallpaperManager.cpp:WE_after_play", "play/pause done", "K");
  // #endregion

  LOG_INFO("Shared wallpaper set successfully!");

  // #region agent log
  debugLog("WallpaperManager.cpp:WE_saveState", "About to call saveState", "K");
  // #endregion
  saveState();
  // #region agent log
  debugLog("WallpaperManager.cpp:WE_saveState_done", "saveState done", "K");
  // #endregion
  writeHyprpaperConfig();
  // #region agent log
  debugLog("WallpaperManager.cpp:WE_complete",
           "Bulk WE setWallpaper complete, returning", "K");
  // #endregion

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
  if (ext == "pkg" || ext == "json" || ext == "html" || ext == "htm") {
    return std::make_shared<WallpaperEngineRenderer>();
  }

  return std::make_shared<StaticRenderer>(); // Default
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
  // Global mute
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
      // Volume typically 0.0 - 1.0 or 0 - 100
      // Assuming 0-100 input, renderer expects float 0-1?
      // Actually BaseRenderer has setVolume(float). Let's assume input is
      // 0-100.
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
  // If global pause is on, resuming single monitor might be conflicted
  // But usually Resume command implies override.
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

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  nlohmann::json wallpapers;

  for (const auto &[name, state] : m_monitors) {
    if (!state.currentPath.empty() &&
        state.currentPath != "TRANSITION_FREEZE") {
      wallpapers[name] = state.currentPath;
    }
  }

  conf.set("wallpapers.current", wallpapers);
  // Persist to disk immediately for reboot persistence
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

  // Check if persistence is enabled (from config, default true)
  // When enabled, WE processes continue running after app closes
  auto &config = config::ConfigManager::getInstance();
  bool persistence =
      config.get<bool>("wallpaper.persistence_on_close", true);

  LOG_INFO("Wallpaper persistence on close: " +
           std::string(persistence ? "enabled" : "disabled"));

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  for (auto &[name, state] : m_monitors) {
    if (state.renderer) {
      if (persistence) {
        // Try to cast to WallpaperEngineRenderer
        auto weRenderer =
            std::dynamic_pointer_cast<WallpaperEngineRenderer>(state.renderer);
        if (weRenderer) {
          weRenderer->detach();
        } else {
          // Static/Video renderers (internal) must stop as they depend on our
          // window
          state.renderer->stop();
        }
      } else {
        state.renderer->stop();
      }
    }
    if (state.window) {
      state.window->hide();
      // Destroy?
    }
  }
  m_monitors.clear();
}

void WallpaperManager::writeHyprpaperConfig() {
#ifndef _WIN32
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
  file << "# This file allows wallpaper to persist without the app "
          "running\n\n";
  file << "splash = false\n";
  file << "ipc = off\n\n";

  std::lock_guard<std::recursive_mutex> lock(m_mutex);

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
    // Check every 5 seconds
    for (int i = 0; i < 50 && !m_stopMonitor; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (m_stopMonitor)
      break;

    uint64_t rss = bwp::utils::SystemUtils::getProcessRSS();
    // Check if paused via config or other means
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
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
}

} // namespace bwp::wallpaper
