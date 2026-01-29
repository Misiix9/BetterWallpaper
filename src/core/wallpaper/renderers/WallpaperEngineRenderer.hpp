#pragma once
#include "../WallpaperRenderer.hpp"
#include <sys/types.h>

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

  WallpaperType getType() const override;

private:
  void terminateProcess();

  pid_t m_pid = -1;
  std::string m_pkPath;
  std::string m_monitor;
  ScalingMode m_mode = ScalingMode::Fill;
  bool m_isPlaying = false;
};

} // namespace bwp::wallpaper
