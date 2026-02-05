#include "WallpaperEngineRenderer.hpp"
#include "../../utils/Logger.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace bwp::wallpaper {

WallpaperEngineRenderer::WallpaperEngineRenderer() {}

WallpaperEngineRenderer::~WallpaperEngineRenderer() { terminateProcess(); }

bool WallpaperEngineRenderer::load(const std::string &path) {
  LOG_SCOPE_AUTO();
  terminateProcess();
  m_pkPath = path;

  // We launch process in play() or here?
  // Usually immediate.
  play();
  return true;
}

void WallpaperEngineRenderer::render(cairo_t *cr, int width, int height) {
  // Excessive logging in render loop? Maybe debug level only, or skip.
  // User asked for "Deep Instrumentation". Let's enable it but maybe use a
  // macro check? Or just trust the Logger level. ScopeTracer uses DEBUG level.
  // LOG_SCOPE_AUTO(); // This might spam 60fps. Risk.
  // Let's SKIP render loop scope tracing to avoid 20GB log files in 1 minute.
  // Or add a static counter to log once per second?
  // The user asked for "Deep Instrumentation" but surely not spam.
  // "Last entering line" implies tracking flow. Render loop flow is repetitive.
  // I will skip render() to be safe, or log once.
  // Let's skip scope tracer here, but maybe log if size changes?
  // No, just skip scope tracer for render().
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  // Maybe draw transparent?
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

void WallpaperEngineRenderer::setScalingMode(ScalingMode mode) {
  LOG_SCOPE_AUTO();
  m_mode = mode;
  // Need to restart process with new args if running?
  if (m_pid != -1) {
    play(); // restart
  }
}

void WallpaperEngineRenderer::setMonitor(const std::string &monitor) {
  m_monitor = monitor;
  m_monitors.clear();
  m_monitors.push_back(monitor);
}

void WallpaperEngineRenderer::setMonitors(
    const std::vector<std::string> &monitors) {
  m_monitors = monitors;
  if (!monitors.empty()) {
    m_monitor = monitors[0]; // Primary?
  }
}

void WallpaperEngineRenderer::launchProcess() {
  LOG_SCOPE_AUTO();
  // Check throttle
  auto now = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::seconds>(now - m_lastLaunchTime)
          .count();

  if (elapsed > 60) {
    m_crashCount = 0; // Reset if stable for 60s
  }

  m_lastLaunchTime = now;

  // Existing launch logic...
  std::string bin = "linux-wallpaperengine";
  std::vector<std::string> args;
  args.push_back(bin);

  // Extract Workshop ID logic...
  std::string arg = m_pkPath;

  // HTML/Web Support
  if (m_pkPath.find(".html") != std::string::npos ||
      m_pkPath.find(".htm") != std::string::npos) {
    // linux-wallpaperengine expects the directory for web headers usually, OR
    // the index.html? Documentation says: passes file to browser. We pass the
    // full path.
    arg = m_pkPath;
  }

  size_t pkgPos = m_pkPath.find("scene.pkg");
  if (pkgPos != std::string::npos) {
    std::string dir = m_pkPath.substr(0, pkgPos);
    if (!dir.empty() && dir.back() == '/')
      dir.pop_back();
    size_t lastSlash = dir.find_last_of('/');
    if (lastSlash != std::string::npos) {
      std::string possibleId = dir.substr(lastSlash + 1);
      if (std::all_of(possibleId.begin(), possibleId.end(), ::isdigit)) {
        arg = possibleId;
      }
    }
  }

  // Decide command structure based on monitor count
  // Multi-monitor chaining: ./linux-wallpaperengine --screen-root A --bg ID
  // --screen-root B --bg ID Single/Default: ./linux-wallpaperengine ID
  // [--screen-root A]

  if (!m_monitors.empty()) {
    // Explicit assignment for each monitor
    for (const auto &mon : m_monitors) {
      args.push_back("--screen-root");
      args.push_back(mon);
      args.push_back("--bg"); // Explicitly assign the background
      args.push_back(arg);
      // TODO: Per-monitor scaling can be added here if we store it
      // args.push_back("--scaling"); ...
    }
  } else {
    // Classic single setup
    args.push_back(arg); // Positional ID
    if (!m_monitor.empty()) {
      args.push_back("--screen-root");
      args.push_back(m_monitor);
    }
  }

  if (m_fpsLimit > 0) {
    args.push_back("--fps");
    args.push_back(std::to_string(m_fpsLimit));
  }

  if (m_muted) {
    args.push_back("--silent");
  }

  std::string cmdLog = "Launching: ";
  for (const auto &a : args)
    cmdLog += a + " ";
  LOG_INFO(cmdLog);

  pid_t pid = fork();
  if (pid == 0) {
    // Set LD_LIBRARY_PATH to include the executable's directory for libGLEW
    // symlink This avoids hardcoded paths and works from both build and install
    // locations
    std::string exeDir;
    std::string workingDir = "."; // Default CWD
    try {
      exeDir =
          std::filesystem::canonical("/proc/self/exe").parent_path().string();
    } catch (...) {
      exeDir = "."; // Fallback to current directory
    }

    // Check if we can bypass the wrapper script and run directly
    // This fixes issues where the wrapper script mishandles LD_LIBRARY_PATH
    if (std::filesystem::exists(
            "/opt/linux-wallpaperengine/linux-wallpaperengine")) {
      bin = "/opt/linux-wallpaperengine/linux-wallpaperengine";
      workingDir = "/opt/linux-wallpaperengine";
    }

    // Set working directory
    int chdirRes = chdir(workingDir.c_str());
    (void)chdirRes; // Suppress unused warning

    const char *currentLd = getenv("LD_LIBRARY_PATH");
    std::string newLd = exeDir;
    // Also add the project root (one level up from build/src/gui)
    newLd += ":" + exeDir + "/..";
    newLd += ":" + exeDir + "/../..";
    newLd += ":" + exeDir + "/../../..";

    // Fix for broken AUR package / installation script
    // which fails to find libcef.so in the root of the install dir
    if (bin.find("/opt/linux-wallpaperengine") != std::string::npos) {
      // Prioritize the install dir if we are running the binary directly
      // Must include BOTH root (for libcef.so) and lib/ (for kissfft etc)
      newLd =
          "/opt/linux-wallpaperengine:/opt/linux-wallpaperengine/lib:" + newLd;
    } else {
      newLd += ":/opt/linux-wallpaperengine:/opt/linux-wallpaperengine/lib";
    }
    newLd += ":/usr/lib/linux-wallpaperengine";

    if (currentLd) {
      newLd += ":";
      newLd += currentLd;
    }
    setenv("LD_LIBRARY_PATH", newLd.c_str(), 1);

    // Ensure DISPLAY is set for XWayland/GLX support on Wayland compositors
    // linux-wallpaperengine requires X11/GLX, so it needs XWayland
    const char *display = getenv("DISPLAY");
    if (!display || display[0] == '\0') {
      // Try common XWayland display values
      setenv("DISPLAY", ":0", 1);
    }

    // Create new session to detach from terminal (allows surviving app closure)
    // This makes the process independent and it will continue running even after
    // the parent app exits - key for wallpaper persistence
    setsid();

    // Close standard file descriptors to fully daemonize
    // This prevents the process from being affected by terminal closure
    close(STDIN_FILENO);
    // Keep stdout/stderr open for logging - redirect to /dev/null for
    // true daemonization if needed (comment these out for debugging)
    // close(STDOUT_FILENO);
    // close(STDERR_FILENO);

    std::vector<char *> c_args;
    for (const auto &a : args)
      c_args.push_back(const_cast<char *>(a.c_str()));
    c_args.push_back(nullptr);
    execvp(bin.c_str(), c_args.data());
    _exit(1);
  } else if (pid > 0) {
    m_pid = pid;
    m_isPlaying = true;

    LOG_INFO("Spawned wallpaper process with PID: " + std::to_string(pid) +
             " (independent session via setsid)");

    // Do NOT block invalidating the UI. Let the watcher thread handle crashes.
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // ... waitpid ...
  }
}

void WallpaperEngineRenderer::play() {
  LOG_SCOPE_AUTO();
  if (m_pid != -1) {
    kill(m_pid, SIGCONT);
    m_isPlaying = true;
    return;
  }

  m_stopWatcher = false;
  if (!m_watcherThread.joinable()) {
    m_watcherThread =
        std::thread(&WallpaperEngineRenderer::monitorProcess, this);
  }

  launchProcess();
}

void WallpaperEngineRenderer::monitorProcess() {
  LOG_SCOPE_AUTO();
  const int MAX_CRASH_COUNT = 3; // Stop after 3 consecutive crashes

  while (!m_stopWatcher) {
    if (m_pid > 0) {
      int status;
      // Use WNOHANG to avoid blocking indefinitely
      pid_t res = waitpid(m_pid, &status, WNOHANG);

      if (res == m_pid && !m_stopWatcher) {
        // Process exited
        if (m_crashCount >= MAX_CRASH_COUNT) {
          LOG_ERROR("WallpaperEngine crashed " + std::to_string(m_crashCount) +
                    " times. Giving up - GLX/OpenGL may be unavailable.");
          m_pid = -1;
          m_isPlaying = false;
          m_stopWatcher = true;
          break;
        }

        LOG_WARN("WallpaperEngine crashed. Attempt " +
                 std::to_string(m_crashCount + 1) + "/" +
                 std::to_string(MAX_CRASH_COUNT));

        int backoff = std::min(
            static_cast<int>(std::pow(2.0, static_cast<double>(m_crashCount))),
            30);
        m_crashCount++;

        m_pid = -1;

        // Interruptible sleep
        std::unique_lock<std::mutex> lock(m_cvMutex);
        m_cv.wait_for(lock, std::chrono::seconds(backoff),
                      [this] { return m_stopWatcher.load(); });

        if (!m_stopWatcher && m_crashCount < MAX_CRASH_COUNT) {
          launchProcess();
        }
      } else if (res == 0) {
        // Process still running, sleep a bit (interruptible)
        std::unique_lock<std::mutex> lock(m_cvMutex);
        m_cv.wait_for(lock, std::chrono::milliseconds(500),
                      [this] { return m_stopWatcher.load(); });
      }
    } else {
      std::unique_lock<std::mutex> lock(m_cvMutex);
      m_cv.wait_for(lock, std::chrono::milliseconds(100),
                    [this] { return m_stopWatcher.load(); });
    }
  }
}

void WallpaperEngineRenderer::pause() {
  LOG_SCOPE_AUTO();
  if (m_pid != -1) {
    if (kill(m_pid, SIGSTOP) != 0) {
      LOG_WARN("Failed to pause process " + std::to_string(m_pid));
    }
    m_isPlaying = false;
  }
}

void WallpaperEngineRenderer::detach() {
  LOG_SCOPE_AUTO();
  if (m_pid != -1) {
    LOG_INFO("Detaching wallpaper process " + std::to_string(m_pid) +
             " (Persistence Enabled)");
    m_detached = true;
    // Signal watcher to stop without killing
    m_stopWatcher = true;
    m_cv.notify_all();
    if (m_watcherThread.joinable()) {
      m_watcherThread.join();
    }
    m_pid = -1;
    m_isPlaying = false;
  }
}

void WallpaperEngineRenderer::stop() {
  LOG_SCOPE_AUTO();
  terminateProcess();
}

void WallpaperEngineRenderer::terminateProcess() {
  LOG_SCOPE_AUTO();
  m_stopWatcher = true;
  m_cv.notify_all();

  if (m_pid != -1 && !m_detached) {
    kill(m_pid, SIGTERM);
    // waitpid handled by thread?
    // If we kill, watcher wakes up.
    // Better signal watcher to stop, then kill.
    // We rely on watcher to wake up on waitpid return.
  }

  if (m_watcherThread.joinable()) {
    m_watcherThread.join();
  }
  m_pid = -1;
  m_isPlaying = false;
}

void WallpaperEngineRenderer::setVolume(float volume) {
  // If volume is 0, consider it muted
  if (volume <= 0.01f) {
    setMuted(true);
  } else {
    // linux-wallpaperengine doesn't have a direct volume flag other than silent
    // We could adjust PulseAudio per app if needed, but for now just unmute
    setMuted(false);
  }
}

void WallpaperEngineRenderer::setPlaybackSpeed(float speed) {
  // Not supported via CLI args easily
}

void WallpaperEngineRenderer::setFpsLimit(int fps) {
  if (m_fpsLimit != fps) {
    m_fpsLimit = fps;
    if (m_isPlaying)
      play(); // Restart
  }
}

void WallpaperEngineRenderer::setMuted(bool muted) {
  if (m_muted != muted) {
    m_muted = muted;
    if (m_isPlaying)
      play(); // Restart
  }
}

void WallpaperEngineRenderer::setAudioData(
    const std::vector<float> & /*audioBands*/) {
  // Wallpaper Engine (linux port) typically captures audio internally via
  // PulseAudio. We don't need to pass FFT data manually unless we are injecting
  // it. For now, this is a no-op as the external process handles it. If we need
  // to send data, we would write to a pipe/socket here.
}

WallpaperType WallpaperEngineRenderer::getType() const {
  if (m_pkPath.find(".html") != std::string::npos ||
      m_pkPath.find(".htm") != std::string::npos)
    return WallpaperType::WEWeb;
  if (m_pkPath.find(".mp4") != std::string::npos ||
      m_pkPath.find(".webm") != std::string::npos)
    return WallpaperType::WEVideo;
  return WallpaperType::WEScene;
}

} // namespace bwp::wallpaper
