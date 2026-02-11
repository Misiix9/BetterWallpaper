#include "StaticRenderer.hpp"
#include "../../utils/Logger.hpp"
#include <gdk/gdk.h>  
namespace bwp::wallpaper {
StaticRenderer::StaticRenderer() {}
StaticRenderer::~StaticRenderer() {
  if (m_pixbuf) {
    g_object_unref(m_pixbuf);
  }
}
bool StaticRenderer::load(const std::string &path) {
  GError *error = nullptr;
  if (m_pixbuf) {
    g_object_unref(m_pixbuf);
    m_pixbuf = nullptr;
  }
  m_pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
  if (!m_pixbuf) {
    LOG_ERROR("Failed to load image " + path + ": " +
              (error ? error->message : "Unknown"));
    if (error)
      g_error_free(error);
    return false;
  }
  m_imgWidth = gdk_pixbuf_get_width(m_pixbuf);
  m_imgHeight = gdk_pixbuf_get_height(m_pixbuf);
  return true;
}
void StaticRenderer::setScalingMode(ScalingMode mode) { m_mode = mode; }
void StaticRenderer::render(cairo_t *cr, int width, int height) {
  if (!m_pixbuf) {
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);
    return;
  }
  double scaleX = (double)width / m_imgWidth;
  double scaleY = (double)height / m_imgHeight;
  double scale = 1.0;
  double tx = 0, ty = 0;
  cairo_save(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
  switch (m_mode) {
  case ScalingMode::Stretch:
    break;  
    break;
  case ScalingMode::Fill:  
    scale = std::max(scaleX, scaleY);
    tx = (width - m_imgWidth * scale) / 2.0;
    ty = (height - m_imgHeight * scale) / 2.0;
    scaleX = scaleY = scale;
    break;
  case ScalingMode::Fit:  
    scale = std::min(scaleX, scaleY);
    tx = (width - m_imgWidth * scale) / 2.0;
    ty = (height - m_imgHeight * scale) / 2.0;
    scaleX = scaleY = scale;
    break;
  case ScalingMode::Center:
    scaleX = scaleY = 1.0;
    tx = (width - m_imgWidth) / 2.0;
    ty = (height - m_imgHeight) / 2.0;
    break;
  case ScalingMode::Zoom:
    scale = std::max(scaleX, scaleY) * 1.2;  
    tx = (width - m_imgWidth * scale) / 2.0;
    ty = (height - m_imgHeight * scale) / 2.0;
    scaleX = scaleY = scale;
    break;
  case ScalingMode::Tile:
    break;
  }
  if (m_mode == ScalingMode::Tile) {
    gdk_cairo_set_source_pixbuf(cr, m_pixbuf, 0, 0);
    cairo_pattern_t *pattern = cairo_get_source(cr);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    cairo_paint(cr);
  } else {
    cairo_save(cr);
    cairo_translate(cr, tx, ty);
    cairo_scale(cr, scaleX, scaleY);
    gdk_cairo_set_source_pixbuf(cr, m_pixbuf, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
  }
}
}  
