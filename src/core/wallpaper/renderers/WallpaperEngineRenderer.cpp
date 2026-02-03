#include "WallpaperEngineRenderer.hpp"
#include "../../utils/Logger.hpp"
#include <algorithm>
#include <cmath>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace bwp::wallpaper {

WallpaperEngineRenderer::WallpaperEngineRenderer() {}

WallpaperEngineRenderer::~WallpaperEngineRenderer() { terminateProcess(); }

bool WallpaperEngineRenderer::load(const std::string &path) {
  terminateProcess();
  m_pkPath = path;

  // We launch process in play() or here?
  // Usually immediate.
  play();
  return true;
}

void WallpaperEngineRenderer::render(cairo_t *cr, int width, int height) {
  // No-op: external process handles rendering
  // Maybe draw transparent?
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

void WallpaperEngineRenderer::setScalingMode(ScalingMode mode) {
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

  pid_t pid = fork();
  if (pid == 0) {
    // Set LD_LIBRARY_PATH to include local directory for libGLEW symlink
    // Find where we are - assume project root for now or relative to
    // executable? Symlink is in /home/onxy/Documents/scripts/BetterWallpaper
    // Let's deduce it or hardcode for this fix, ideally use a relative path
    // logic. But standard way: just getenv, append, setenv.

    const char *currentLd = getenv("LD_LIBRARY_PATH");
    std::string newLd =
        "/home/onxy/Documents/scripts/BetterWallpaper"; // Hardcode for
                                                        // immediate fix as user
                                                        // is stuck here
    if (currentLd) {
      newLd += ":";
      newLd += currentLd;
    }
    setenv("LD_LIBRARY_PATH", newLd.c_str(), 1);

    std::vector<char *> c_args;
    for (const auto &a : args)
      c_args.push_back(const_cast<char *>(a.c_str()));
    c_args.push_back(nullptr);
    execvp(bin.c_str(), c_args.data());
    _exit(1);
  } else if (pid > 0) {
    m_pid = pid;
    m_isPlaying = true;

    m_pid = pid;
    m_isPlaying = true;

    // Do NOT block invalidating the UI. Let the watcher thread handle crashes.
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // ... waitpid ...
  }
}

void WallpaperEngineRenderer::play() {
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
  if (m_pid != -1) {
    if (kill(m_pid, SIGSTOP) != 0) {
      LOG_WARN("Failed to pause process " + std::to_string(m_pid));
    }
    m_isPlaying = false;
  }
}

void WallpaperEngineRenderer::stop() { terminateProcess(); }

void WallpaperEngineRenderer::terminateProcess() {
  m_stopWatcher = true;
  m_cv.notify_all();

  if (m_pid != -1) {
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
  return WallpaperType::WEScene; // Or Video, assuming Scene for now
}

} // namespace bwp::wallpaper
