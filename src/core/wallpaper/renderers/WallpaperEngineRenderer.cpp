#include "WallpaperEngineRenderer.hpp"
#include "../../utils/Logger.hpp"
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
}

void WallpaperEngineRenderer::play() {
  if (m_pid != -1) {
    // Maybe resume signal?
    kill(m_pid, SIGCONT);
    m_isPlaying = true;
    return;
  }

  // Launch linux-wallpaperengine
  std::string bin = "linux-wallpaperengine"; // Assumed in PATH

  std::vector<std::string> args;
  args.push_back(bin);
  args.push_back("--assets-dir");
  args.push_back(m_pkPath);

  if (!m_monitor.empty()) {
    args.push_back("--screen-root");
    args.push_back(m_monitor);
  }

  // Fork
  pid_t pid = fork();
  if (pid == 0) {
    // Child
    std::vector<char *> c_args;
    for (const auto &arg : args)
      c_args.push_back(const_cast<char *>(arg.c_str()));
    c_args.push_back(nullptr);

    execvp(bin.c_str(), c_args.data());
    _exit(1); // Exec failed
  } else if (pid > 0) {
    m_pid = pid;
    m_isPlaying = true;
  } else {
    LOG_ERROR("Failed to fork linux-wallpaperengine");
  }
}

void WallpaperEngineRenderer::pause() {
  if (m_pid != -1) {
    kill(m_pid, SIGSTOP); // Simple pause
    m_isPlaying = false;
  }
}

void WallpaperEngineRenderer::stop() { terminateProcess(); }

void WallpaperEngineRenderer::terminateProcess() {
  if (m_pid != -1) {
    kill(m_pid, SIGTERM);
    waitpid(m_pid, nullptr, 0); // Blocking wait?
    m_pid = -1;
    m_isPlaying = false;
  }
}

void WallpaperEngineRenderer::setVolume(float volume) {
  // Cannot easily control without IPC to LWP
}

void WallpaperEngineRenderer::setPlaybackSpeed(float speed) {
  // Cannot easily control without IPC to LWP
}

WallpaperType WallpaperEngineRenderer::getType() const {
  return WallpaperType::WEScene; // Or Video, assuming Scene for now
}

} // namespace bwp::wallpaper
