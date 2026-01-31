#pragma once
#include "../WallpaperRenderer.hpp"
#include <atomic>
#include <chrono>
#include <sys/types.h>
#include <thread>

namespace bwp::wallpaper {

class WallpaperEngineRenderer : public WallpaperRenderer {
public:
  WallpaperEngineRenderer();
  ~WallpaperEngineRenderer() override;

  bool load(const std::string &path) override;
  void render(cairo_t *cr, int width, int height) override;

  void setScalingMode(ScalingMode mode) override;
  void setMonitor(const std::string &monitor) override;

  void play() override;
  void pause() override;
  void stop() override;

  void setVolume(float volume) override;
  void setPlaybackSpeed(float speed) override;

  void setFpsLimit(int fps);
  void setMuted(bool muted);

  WallpaperType getType() const override;

private:
  void terminateProcess();
  void monitorProcess();
  void launchProcess();

  std::thread m_watcherThread;
  std::atomic<bool> m_stopWatcher = false;
  std::atomic<int> m_crashCount = 0;
  std::chrono::steady_clock::time_point m_lastLaunchTime;

  pid_t m_pid = -1;
  std::string m_pkPath;
  std::string m_monitor;

  ScalingMode m_mode = ScalingMode::Fill;
  bool m_isPlaying = false;

  int m_fpsLimit = 0; // 0 = unlimited
  bool m_muted = false;
};

} // namespace bwp::wallpaper
