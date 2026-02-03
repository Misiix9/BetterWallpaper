#pragma once
#include "WallpaperInfo.hpp"
#ifdef _WIN32
// Stub cairo type for Windows
typedef struct _cairo cairo_t;
#else
#include <cairo.h>
#endif
#include <string>

namespace bwp::wallpaper {

class WallpaperRenderer {
public:
  virtual ~WallpaperRenderer() = default;

  virtual bool load(const std::string &path) = 0;
  virtual void render(cairo_t *cr, int width, int height) = 0;

  virtual void setScalingMode(ScalingMode mode) = 0;
  virtual void setMonitor(const std::string &monitor) {}
  virtual void setMonitors(const std::vector<std::string> &monitors) {}
  virtual void update(double dt) {} // For animations

  virtual void play() {}
  virtual void pause() {}
  virtual void stop() {}

  virtual void setVolume(float volume) {}
  virtual void setPlaybackSpeed(float speed) {}
  virtual void setMuted(bool muted) {}

  // Audio visualization data (frequency bands, normalized 0-1)
  virtual void setAudioData(const std::vector<float> & /*audioBands*/) {}

  virtual bool isPlaying() const { return false; }
  virtual bool hasAudio() const { return false; }

  virtual WallpaperType getType() const = 0;
};

} // namespace bwp::wallpaper
