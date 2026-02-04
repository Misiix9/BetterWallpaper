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
  // Determine type and create renderer
  auto renderer = createRenderer(path);
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

  LOG_INFO("Loaded wallpaper successfully"); // EXPERIMENTAL: Screenshot Freeze
                                             // Transition
  // 1. Capture current state
  std::string screenshotPath = "/tmp/bwp_freeze_" + monitorName + ".png";
  std::string cmd = "grim -o " + monitorName + " " + screenshotPath;

  // Use runCommand to capture. Ignoring return code for now, assuming grim
  // exists.
  bwp::utils::SystemUtils::runCommand(cmd);

  // 2. Create StaticRenderer from screenshot
  auto freezeRenderer = std::make_shared<StaticRenderer>();
  if (freezeRenderer->load(screenshotPath)) {
    // 3. Set Freeze renderer immediately
    LOG_INFO("Freezing screen with screenshot: " + screenshotPath);
    it->second.renderer = freezeRenderer;
    it->second.window->setRenderer(freezeRenderer);
    it->second.currentPath = "TRANSITION_FREEZE"; // usage marker
  } else {
    LOG_WARN("Failed to load freeze screenshot, skipping freeze step.");
    // If fail, just proceed normal flow (will cause black flash)
  }

  // 4. Launch Target Renderer (starts process)
  // already created `renderer` above
  renderer->setMonitor(monitorName);
  // Re-load? No, we haven't loaded it yet. Logic above says "createRenderer"
  // then "load". Wait, I am replacing lines 189-237. The original code passed
  // `renderer` (created but not loaded) to this block? Let's check
  // `WallpaperManager.cpp`. Line 172: `createRenderer`. Line 189:
  // `renderer->load(path)`. My patch STARTS at line 200 (Stop old). So
  // `renderer` IS ALREADY LOADED and Process STARTED at line 189! So the
  // process is running. If I set `freezeRenderer` NOW, it overlays the
  // just-started process (which is black). This is correct. `freezeRenderer`
  // (Static) is drawn by `WallpaperWindow` (GTK), which is likely TOP layer of
  // the wallpaper plane. `linux-wallpaperengine` is BOTTOM layer. So GTK
  // Overlay covers black WE window.

  // 5. Spawn thread to wait and reveal
  std::thread([this, monitorName, freezeRenderer, renderer, path]() {
  // Wait for WE to initialize GL and show content
#ifdef _WIN32
    Sleep(2000);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
#endif

    std::lock_guard<std::recursive_mutex> asyncLock(m_mutex);

    // Check if we are still the active renderer (handler for rapid switching)
    auto &state = m_monitors[monitorName];
    if (state.renderer == freezeRenderer) {
      LOG_INFO("Revealing new wallpaper via transition");
      state.renderer = renderer;
      state.currentPath = path;
      // Smooth fade from Screenshot -> Transparent (revealing WE)
      state.window->transitionTo(renderer);

      if (m_paused)
        renderer->pause();
      else
        renderer->play();

      if (m_scalingModes.count(monitorName)) {
        renderer->setScalingMode(
            static_cast<ScalingMode>(m_scalingModes[monitorName]));
      }
    } else {
      LOG_INFO("Wallpaper changed during transition, aborting reveal");
      // `renderer` will be destroyed (if no other refs), causing
      // `terminateProcess`
    }
  }).detach();

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

void WallpaperManager::shutdown() {
  LOG_INFO("Shutting down WallpaperManager...");
  stopResourceMonitor();

  // Check if persistence is enabled (default true)
  // We could make this a config option: "general.persistence"
  bool persistence = true;

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
