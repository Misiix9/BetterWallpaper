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

  // Stub GtkWindow* as void* or just remove getWindow() usage if possible
  void* getWindow() { return nullptr; }

  void show() {}
  void hide() {}

  void updateMonitor(const monitor::MonitorInfo &monitor) {}
};
#else
class WallpaperWindow {
public:
  WallpaperWindow(const monitor::MonitorInfo &monitor);
  ~WallpaperWindow();

  void setRenderer(
      std::weak_ptr<WallpaperRenderer> renderer); // Weak ptr? Or shared.
  void transitionTo(std::shared_ptr<WallpaperRenderer> nextRenderer);
  void render(); // Called on frame tick

  GtkWindow *getWindow() { return GTK_WINDOW(m_window); }

  void show();
  void hide();

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
};
#endif

} // namespace bwp::wallpaper
