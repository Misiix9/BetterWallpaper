#pragma once
#include "../WallpaperRenderer.hpp"
#include <cairo.h>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mutex>

namespace bwp::wallpaper {

class VideoRenderer : public WallpaperRenderer {
public:
  VideoRenderer();
  ~VideoRenderer() override;

  bool load(const std::string &path) override;
  void render(cairo_t *cr, int width, int height) override;
  void setScalingMode(ScalingMode mode) override;

  void play() override;
  void pause() override;
  void stop() override;

  void setVolume(float volume) override;
  void setPlaybackSpeed(float speed) override;

  bool isPlaying() const override { return !m_paused; }
  bool hasAudio() const override { return true; }

  WallpaperType getType() const override { return WallpaperType::Video; }

private:
  mpv_handle *m_mpv = nullptr;
  mpv_render_context *m_mpv_ctx = nullptr;
  ScalingMode m_mode = ScalingMode::Fill;
  bool m_paused = false;

  // Software buffer for cairo
  uint8_t *m_buffer = nullptr;
  int m_bufWidth = 0;
  int m_bufHeight = 0;
};

} // namespace bwp::wallpaper
