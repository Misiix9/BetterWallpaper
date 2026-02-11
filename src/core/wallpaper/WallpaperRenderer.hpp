#pragma once
#include "WallpaperInfo.hpp"
#ifdef _WIN32
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
  virtual void setMonitor(const std::string &  ) {}
  virtual void setMonitors(const std::vector<std::string> &  ) {}
  virtual void update(double  ) {}  
  virtual void play() {}
  virtual void pause() {}
  virtual void stop() {}
  virtual void setVolume(float  ) {}
  virtual void setPlaybackSpeed(float  ) {}
  virtual void setMuted(bool  ) {}
  virtual void setAudioData(const std::vector<float> &  ) {}
  virtual bool isPlaying() const { return false; }
  virtual bool hasAudio() const { return false; }
  virtual WallpaperType getType() const = 0;
};
}  
