#pragma once
#include "../monitor/MonitorInfo.hpp"
#include "../transition/TransitionEngine.hpp"
#include "WallpaperRenderer.hpp"
#include <gtk/gtk.h>
#include <memory>

namespace bwp::wallpaper {

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

} // namespace bwp::wallpaper
