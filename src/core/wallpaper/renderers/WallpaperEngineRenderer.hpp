#pragma once
#include "../WallpaperRenderer.hpp"
#include <atomic>
#include <chrono>
#ifndef _WIN32
#include <sys/types.h>
#endif
#include <condition_variable>
#include <mutex>
#include <thread>
namespace bwp::wallpaper {
#ifdef _WIN32
class WallpaperEngineRenderer : public WallpaperRenderer {
public:
  WallpaperEngineRenderer() {}
  ~WallpaperEngineRenderer() override {}
  bool load(const std::string &path) override { return true; }
  void render(cairo_t *cr, int width, int height) override {}
  void setScalingMode(ScalingMode mode) override {}
  void setMonitor(const std::string &monitor) override {}
  void play() override {}
  void pause() override {}
  void stop() override {}
  bool isPlaying() const override { return false; }
  void setVolume(float volume) override {}
  void setPlaybackSpeed(float speed) override {}
  void setAudioData(const std::vector<float> &audioBands) override {}
  void setFpsLimit(int fps) {}
  void setMuted(bool muted) {}
  WallpaperType getType() const override { return WallpaperType::WEScene; }
};
#else
class WallpaperEngineRenderer : public WallpaperRenderer {
public:
  WallpaperEngineRenderer();
  ~WallpaperEngineRenderer() override;
  bool load(const std::string &path) override;
  void render(cairo_t *cr, int width, int height) override;
  void setScalingMode(ScalingMode mode) override;
  void setMonitor(const std::string &monitor) override;
  void setMonitors(const std::vector<std::string> &monitors) override;
  void play() override;
  void pause() override;
  void stop() override;
  bool isPlaying() const override;
  void detach();
  void prepareForReplacement() override;
  void setVolume(float volume) override;
  void setPlaybackSpeed(float speed) override;
  void setAudioData(const std::vector<float> &audioBands) override;
  void setFpsLimit(int fps);
  void setMuted(bool muted) override;
  void setNoAudioProcessing(bool enabled);
  void setDisableMouse(bool enabled);
  void setNoAutomute(bool enabled);
  void setVolumeLevel(int volume);
  WallpaperType getType() const override;
  bool isReady() const override;

  /// Kill all linux-wallpaperengine processes targeting specific monitors.
  /// Scans /proc to find orphaned/zombie processes from previous sessions.
  static void killExistingProcessesForMonitors(
      const std::vector<std::string> &monitors);
  /// Kill ALL linux-wallpaperengine processes (used at daemon startup).
  static void killAllExistingProcesses();

private:
  void terminateProcess();
  void monitorProcess();
  void launchProcess();
  std::thread m_watcherThread;
  std::atomic<bool> m_stopWatcher = false;
  std::atomic<bool> m_detached = false;
  std::atomic<int> m_crashCount = 0;
  std::chrono::steady_clock::time_point m_lastLaunchTime;
  std::chrono::steady_clock::time_point m_startTime;
  std::mutex m_cvMutex;
  std::condition_variable m_cv;
  pid_t m_pid = -1;
  std::string m_pkPath;
  std::string m_monitor;
  std::vector<std::string> m_monitors;
  ScalingMode m_mode = ScalingMode::Fill;
  bool m_isPlaying = false;
  int m_fpsLimit = 60;
  bool m_muted = false;
  bool m_noAudioProcessing = false;
  bool m_disableMouse = false;
  bool m_noAutomute = false;
  int m_volumeLevel = 50;
};
#endif
} // namespace bwp::wallpaper
