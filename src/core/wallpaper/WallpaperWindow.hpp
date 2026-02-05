#pragma once
#include "../monitor/MonitorInfo.hpp"
#include "../transition/TransitionEngine.hpp"
#include "WallpaperRenderer.hpp"
#ifndef _WIN32
#include <gtk/gtk.h>
#endif
#include <memory>

namespace bwp::wallpaper {

#ifdef _WIN32
class WallpaperWindow {
public:
  WallpaperWindow(const monitor::MonitorInfo &monitor) {}
  ~WallpaperWindow() {}

  void setRenderer(std::weak_ptr<WallpaperRenderer> renderer) {}
  void transitionTo(std::shared_ptr<WallpaperRenderer> nextRenderer) {}
  void render() {}

  void *getWindow() { return nullptr; }

  void show() {}
  void hide() {}
  void setOpacity(double opacity) {}
  double getOpacity() const { return 1.0; }

  void updateMonitor(const monitor::MonitorInfo &monitor) {}
};
#else
class WallpaperWindow {
public:
  WallpaperWindow(const monitor::MonitorInfo &monitor);
  ~WallpaperWindow();

  void setRenderer(std::weak_ptr<WallpaperRenderer> renderer);
  void transitionTo(std::shared_ptr<WallpaperRenderer> nextRenderer);
  void render();

  GtkWindow *getWindow() { return GTK_WINDOW(m_window); }

  void show();
  void hide();

  /**
   * Set the window opacity (0.0 = invisible, 1.0 = fully visible)
   * Used for transitions where new wallpaper fades in over old one
   */
  void setOpacity(double opacity);
  double getOpacity() const;

  void updateMonitor(const monitor::MonitorInfo &monitor);

private:
  static gboolean onExtractFrame(GtkWidget *widget, GdkFrameClock *clock,
                                 gpointer user_data);
  static void onDraw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
                     gpointer user_data);

  GtkWidget *m_window = nullptr;
  GtkWidget *m_drawingArea = nullptr;
  monitor::MonitorInfo m_monitor;
  std::weak_ptr<WallpaperRenderer> m_renderer;
  bwp::transition::TransitionEngine m_transitionEngine;
  double m_opacity = 1.0;
};
#endif

} // namespace bwp::wallpaper
