#pragma once
#include "../WallpaperRenderer.hpp"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <memory>

namespace bwp::wallpaper {

class StaticRenderer : public WallpaperRenderer {
public:
  StaticRenderer();
  ~StaticRenderer() override;

  bool load(const std::string &path) override;
  void render(cairo_t *cr, int width, int height) override;
  void setScalingMode(ScalingMode mode) override;

  WallpaperType getType() const override { return WallpaperType::StaticImage; }

private:
  ScalingMode m_mode = ScalingMode::Fill;
  GdkPixbuf *m_pixbuf = nullptr;
  int m_imgWidth = 0;
  int m_imgHeight = 0;
};

} // namespace bwp::wallpaper
