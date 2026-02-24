#include "WallpaperEngineRenderer.hpp"
#include "../../utils/Logger.hpp"
#include <algorithm>
#include <cmath>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
namespace bwp::wallpaper {
WallpaperEngineRenderer::WallpaperEngineRenderer() {}
WallpaperEngineRenderer::~WallpaperEngineRenderer() { terminateProcess(); }

void WallpaperEngineRenderer::killAllExistingProcesses() {
  LOG_INFO("Killing ALL lingering linux-wallpaperengine processes");
  pid_t myPid = getpid();
  DIR *procDir = opendir("/proc");
  if (!procDir)
    return;
  int killed = 0;
  struct dirent *entry;
  while ((entry = readdir(procDir)) != nullptr) {
    // Only look at numeric directory names (PIDs)
    if (entry->d_type != DT_DIR)
      continue;
    bool allDigits = true;
    for (const char *p = entry->d_name; *p; ++p) {
      if (*p < '0' || *p > '9') {
        allDigits = false;
        break;
      }
    }
    if (!allDigits)
      continue;
    pid_t pid = static_cast<pid_t>(std::stoi(entry->d_name));
    if (pid == myPid)
      continue;
    std::string cmdlinePath = std::string("/proc/") + entry->d_name + "/cmdline";
    std::ifstream cmdFile(cmdlinePath, std::ios::binary);
    if (!cmdFile)
      continue;
    std::string cmdline;
    std::getline(cmdFile, cmdline, '\0'); // first arg = binary name
    if (cmdline.find("linux-wallpaperengine") == std::string::npos)
      continue;
    LOG_INFO("Killing orphaned WE process PID " + std::to_string(pid));
    kill(pid, SIGTERM);
    killed++;
  }
  closedir(procDir);
  if (killed > 0) {
    // Give processes time to exit gracefully
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    LOG_INFO("Killed " + std::to_string(killed) + " orphaned WE processes");
  }
}

void WallpaperEngineRenderer::killExistingProcessesForMonitors(
    const std::vector<std::string> &monitors) {
  if (monitors.empty())
    return;
  pid_t myPid = getpid();
  DIR *procDir = opendir("/proc");
  if (!procDir)
    return;
  int killed = 0;
  struct dirent *entry;
  while ((entry = readdir(procDir)) != nullptr) {
    if (entry->d_type != DT_DIR)
      continue;
    bool allDigits = true;
    for (const char *p = entry->d_name; *p; ++p) {
      if (*p < '0' || *p > '9') {
        allDigits = false;
        break;
      }
    }
    if (!allDigits)
      continue;
    pid_t pid = static_cast<pid_t>(std::stoi(entry->d_name));
    if (pid == myPid)
      continue;
    // Read entire cmdline (NUL-separated args)
    std::string cmdlinePath = std::string("/proc/") + entry->d_name + "/cmdline";
    std::ifstream cmdFile(cmdlinePath, std::ios::binary);
    if (!cmdFile)
      continue;
    std::string raw((std::istreambuf_iterator<char>(cmdFile)),
                    std::istreambuf_iterator<char>());
    if (raw.find("linux-wallpaperengine") == std::string::npos)
      continue;
    // Check if any target monitor appears after --screen-root
    // cmdline is NUL-separated: convert to space-separated for matching
    std::string cmdline = raw;
    std::replace(cmdline.begin(), cmdline.end(), '\0', ' ');
    bool targetsOurMonitor = false;
    for (const auto &mon : monitors) {
      // Look for "--screen-root <monitor>" in the command line
      std::string pattern = "--screen-root " + mon;
      if (cmdline.find(pattern) != std::string::npos) {
        targetsOurMonitor = true;
        break;
      }
    }
    if (!targetsOurMonitor)
      continue;
    LOG_INFO("Killing existing WE process PID " + std::to_string(pid) +
             " targeting monitored output");
    kill(pid, SIGTERM);
    killed++;
  }
  closedir(procDir);
  if (killed > 0) {
    // Give processes time to exit gracefully, then SIGKILL stragglers
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_INFO("Killed " + std::to_string(killed) +
             " existing WE processes for target monitors");
  }
}
bool WallpaperEngineRenderer::load(const std::string &path) {
  LOG_SCOPE_AUTO();
  if (path.empty()) {
    LOG_ERROR("Cannot load wallpaper: empty path provided");
    return false;
  }
  terminateProcess();
  m_pkPath = path;
  m_crashCount = 0;
  play();
  return true;
}
void WallpaperEngineRenderer::render(cairo_t *cr, int, int) {
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}
void WallpaperEngineRenderer::setScalingMode(ScalingMode mode) {
  LOG_SCOPE_AUTO();
  m_mode = mode;
  if (m_pid != -1) {
    play();
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
    m_monitor = monitors[0];
  }
}
void WallpaperEngineRenderer::launchProcess() {
  LOG_SCOPE_AUTO();
  if (m_pkPath.empty()) {
    LOG_ERROR("Cannot launch process: no wallpaper path set");
    return;
  }
  auto now = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::seconds>(now - m_lastLaunchTime)
          .count();
  if (elapsed > 60) {
    m_crashCount = 0;
  }
  m_lastLaunchTime = now;
  m_startTime = now; // Track when we started launching
  std::string bin = "linux-wallpaperengine";
  std::vector<std::string> args;
  args.push_back(bin);
  // Add --set-property camerafade=false to disable linux-wallpaperengine's
  // internal fade
  args.push_back("--set-property");
  args.push_back("camerafade=false");
  std::string arg = m_pkPath;
  if (m_pkPath.find(".html") != std::string::npos ||
      m_pkPath.find(".htm") != std::string::npos) {
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
  if (arg.empty()) {
    LOG_ERROR("Cannot launch: wallpaper arg resolved to empty string");
    return;
  }
  args.push_back(arg);
  if (!m_monitors.empty()) {
    for (const auto &mon : m_monitors) {
      args.push_back("--screen-root");
      args.push_back(mon);
    }
  } else if (!m_monitor.empty()) {
    args.push_back("--screen-root");
    args.push_back(m_monitor);
  }
  if (m_fpsLimit > 0) {
    args.push_back("--fps");
    args.push_back(std::to_string(m_fpsLimit));
  }
  if (m_muted) {
    args.push_back("--silent");
  }
  if (m_noAudioProcessing) {
    args.push_back("--no-audio-processing");
  }
  if (m_disableMouse) {
    args.push_back("--disable-mouse");
  }
  if (m_noAutomute) {
    args.push_back("--noautomute");
  }
  if (!m_muted && m_volumeLevel >= 0 && m_volumeLevel <= 100) {
    args.push_back("--volume");
    args.push_back(std::to_string(m_volumeLevel));
  }
  std::string cmdLog = "Launching: ";
  for (const auto &a : args)
    cmdLog += a + " ";
  LOG_INFO(cmdLog);

  // Kill any existing WE processes targeting the same monitor(s)
  // This prevents zombie processes from previous daemon sessions
  killExistingProcessesForMonitors(m_monitors.empty()
      ? std::vector<std::string>{m_monitor}
      : m_monitors);

  pid_t pid = fork();
  if (pid == 0) {
    std::string exeDir;
    std::string workingDir = ".";
    try {
      exeDir =
          std::filesystem::canonical("/proc/self/exe").parent_path().string();
    } catch (...) {
      exeDir = ".";
    }
    if (std::filesystem::exists(
            "/opt/linux-wallpaperengine/linux-wallpaperengine")) {
      bin = "/opt/linux-wallpaperengine/linux-wallpaperengine";
      workingDir = "/opt/linux-wallpaperengine";
    }
    int chdirRes = chdir(workingDir.c_str());
    (void)chdirRes;
    const char *currentLd = getenv("LD_LIBRARY_PATH");
    std::string newLd = exeDir;
    newLd += ":" + exeDir + "/..";
    newLd += ":" + exeDir + "/../..";
    newLd += ":" + exeDir + "/../../..";
    if (bin.find("/opt/linux-wallpaperengine") != std::string::npos) {
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
    const char *display = getenv("DISPLAY");
    if (!display || display[0] == '\0') {
      setenv("DISPLAY", ":0", 1);
    }
    const char *sessionType = getenv("XDG_SESSION_TYPE");
    if (!sessionType || sessionType[0] == '\0') {
      const char *waylandDisplay = getenv("WAYLAND_DISPLAY");
      if (waylandDisplay && waylandDisplay[0] != '\0') {
        setenv("XDG_SESSION_TYPE", "wayland", 1);
      } else {
        setenv("XDG_SESSION_TYPE", "x11", 1);
      }
    }
    const char *waylandDisp = getenv("WAYLAND_DISPLAY");
    if (!waylandDisp || waylandDisp[0] == '\0') {
      const char *xdgRuntime = getenv("XDG_RUNTIME_DIR");
      if (xdgRuntime) {
        std::string socketPath = std::string(xdgRuntime) + "/wayland-1";
        if (std::filesystem::exists(socketPath)) {
          setenv("WAYLAND_DISPLAY", "wayland-1", 1);
        } else {
          socketPath = std::string(xdgRuntime) + "/wayland-0";
          if (std::filesystem::exists(socketPath)) {
            setenv("WAYLAND_DISPLAY", "wayland-0", 1);
          }
        }
      }
    }
    setsid();
    close(STDIN_FILENO);
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
  }
}
void WallpaperEngineRenderer::play() {
  LOG_SCOPE_AUTO();
  if (m_pkPath.empty()) {
    LOG_WARN("play() called but no wallpaper path set — ignoring");
    return;
  }
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
  const int MAX_CRASH_COUNT = 3;
  while (!m_stopWatcher) {
    if (m_pid > 0) {
      int status;
      pid_t res = waitpid(m_pid, &status, WNOHANG);
      if (res == m_pid && !m_stopWatcher) {
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
        std::unique_lock<std::mutex> lock(m_cvMutex);
        m_cv.wait_for(lock, std::chrono::seconds(backoff),
                      [this] { return m_stopWatcher.load(); });
        if (!m_stopWatcher && m_crashCount < MAX_CRASH_COUNT &&
            !m_pkPath.empty()) {
          launchProcess();
        }
      } else if (res == 0) {
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
void WallpaperEngineRenderer::prepareForReplacement() {
  LOG_INFO("Disabling auto-restart for renderer (PID: " +
           std::to_string(m_pid) + ") — replacement incoming");
  m_stopWatcher = true;
  m_cv.notify_all();
}

void WallpaperEngineRenderer::detach() {
  LOG_SCOPE_AUTO();
  if (m_pid != -1) {
    LOG_INFO("Detaching wallpaper process " + std::to_string(m_pid) +
             " (Persistence Enabled)");
    m_detached = true;
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
bool WallpaperEngineRenderer::isPlaying() const {
  return m_isPlaying && m_pid != -1;
}
void WallpaperEngineRenderer::terminateProcess() {
  LOG_SCOPE_AUTO();
  m_stopWatcher = true;
  m_cv.notify_all();
  pid_t pidToReap = m_pid;
  if (pidToReap != -1 && !m_detached) {
    kill(pidToReap, SIGTERM);
  }
  if (m_watcherThread.joinable()) {
    m_watcherThread.join();
  }
  if (pidToReap != -1 && !m_detached) {
    int status = 0;
    for (int i = 0; i < 20; ++i) {
      pid_t res = waitpid(pidToReap, &status, WNOHANG);
      if (res == pidToReap || res == -1) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (waitpid(pidToReap, &status, WNOHANG) == 0) {
      LOG_WARN("Process " + std::to_string(pidToReap) +
               " did not exit after SIGTERM, sending SIGKILL");
      kill(pidToReap, SIGKILL);
      waitpid(pidToReap, &status, 0);
    }
  }
  m_pid = -1;
  m_isPlaying = false;
}
void WallpaperEngineRenderer::setVolume(float volume) {
  int vol = static_cast<int>(volume * 100.0f);
  setVolumeLevel(vol);
}
void WallpaperEngineRenderer::setPlaybackSpeed(float) {}
void WallpaperEngineRenderer::setFpsLimit(int fps) {
  if (m_fpsLimit != fps) {
    m_fpsLimit = fps;
    if (m_isPlaying && m_pid != -1) {
      terminateProcess();
      m_crashCount = 0;
      play();
    }
  }
}
void WallpaperEngineRenderer::setMuted(bool muted) { m_muted = muted; }
void WallpaperEngineRenderer::setNoAudioProcessing(bool enabled) {
  m_noAudioProcessing = enabled;
}
void WallpaperEngineRenderer::setDisableMouse(bool enabled) {
  m_disableMouse = enabled;
}
void WallpaperEngineRenderer::setNoAutomute(bool enabled) {
  m_noAutomute = enabled;
}
void WallpaperEngineRenderer::setVolumeLevel(int volume) {
  volume = std::clamp(volume, 0, 100);
  m_volumeLevel = volume;
}
void WallpaperEngineRenderer::setAudioData(const std::vector<float> &) {}
WallpaperType WallpaperEngineRenderer::getType() const {
  if (m_pkPath.find(".html") != std::string::npos ||
      m_pkPath.find(".htm") != std::string::npos)
    return WallpaperType::WEWeb;
  if (m_pkPath.find(".mp4") != std::string::npos ||
      m_pkPath.find(".webm") != std::string::npos)
    return WallpaperType::WEVideo;
  return WallpaperType::WEScene;
}
bool WallpaperEngineRenderer::isReady() const {
  if (m_pid <= 0)
    return false;
  auto now = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime)
          .count();
  // Wait at least 1000ms for process to initialize and render first frame
  return elapsed > 1000;
}
} // namespace bwp::wallpaper
