#pragma once
#include "../monitor/MonitorInfo.hpp"
#include "../transition/TransitionEngine.hpp"
#include "WallpaperRenderer.hpp"
#ifndef _WIN32
#include <gtk/gtk.h>
#include <cairo.h>
#endif
#include <functional>
#include <memory>
namespace bwp::wallpaper {
#ifdef _WIN32
class WallpaperWindow {
public:
  WallpaperWindow(const monitor::MonitorInfo &monitor) {}
  ~WallpaperWindow() {}
  void setRenderer(std::weak_ptr<WallpaperRenderer> renderer) {}
  void transitionTo(std::shared_ptr<WallpaperRenderer> nextRenderer,
                    std::function<void()> onComplete = nullptr,
                    bool isRetry = false) {}
  void render() {}
  void *getWindow() { return nullptr; }
  void show() {}
  void hide() {}
  void raiseToTopForTransition() {}
  void getTransitionDimensions(int *, int *) const {}
  void startTransitionWithSurfaces(void *, void *,
                                   std::shared_ptr<WallpaperRenderer>,
                                   std::function<void()>) {}
  void prepareTransitionAndStart(std::shared_ptr<WallpaperRenderer>,
                                 std::function<void()>) {}
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
  void transitionTo(std::shared_ptr<WallpaperRenderer> nextRenderer,
                    std::function<void()> onComplete = nullptr,
                    bool isRetry = false);
  void render();
  GtkWindow *getWindow() { return GTK_WINDOW(m_window); }
  void show();
  void hide();
  void raiseToTopForTransition();
  void getTransitionDimensions(int *outWidth, int *outHeight) const;
  void startTransitionWithSurfaces(cairo_surface_t *from, cairo_surface_t *to,
                                   std::shared_ptr<WallpaperRenderer> nextRenderer,
                                   std::function<void()> onComplete);
  /** Build from/to at BACKGROUND (A still visible), then raise to TOP and start transition. Use for non-WE so we never show blank. */
  void prepareTransitionAndStart(std::shared_ptr<WallpaperRenderer> nextRenderer,
                                 std::function<void()> onComplete);
  void setOpacity(double opacity);
  double getOpacity() const;
  std::shared_ptr<WallpaperRenderer> getRenderer() const;
  void updateMonitor(const monitor::MonitorInfo &monitor);

private:
  static gboolean onExtractFrame(GtkWidget *widget, GdkFrameClock *clock,
                                 gpointer user_data);
  static gboolean onTransitionTick(gpointer user_data);
  static gboolean onLowerLayerToBackground(gpointer user_data);
  static gboolean onHideForWallpaperEngine(gpointer user_data);
  static gboolean onTransitionToRetry(gpointer user_data);
  void transitionTo(std::shared_ptr<WallpaperRenderer> nextRenderer,
                    std::function<void()> onComplete, bool isRetry, int retryCount);
  void transitionToPhase2(std::shared_ptr<WallpaperRenderer> nextRenderer,
                          std::function<void()> onComplete, int retryCount);
  static gboolean onTransitionToPhase2Idle(gpointer user_data);
  static void onDraw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
                     gpointer user_data);
  GtkWidget *m_window = nullptr;
  GtkWidget *m_drawingArea = nullptr;
  monitor::MonitorInfo m_monitor;
  std::weak_ptr<WallpaperRenderer> m_renderer;
  bwp::transition::TransitionEngine m_transitionEngine;
  double m_opacity = 1.0;
  int m_transitionFrameCount = 0;
};
#endif
} // namespace bwp::wallpaper
