#include "WallpaperWindow.hpp"
#include "../config/ConfigManager.hpp"
#include "../monitor/MonitorInfo.hpp"
#include "../transition/EffectFactory.hpp"
#include "../transition/Easing.hpp"
#include "../utils/Logger.hpp"
#include "renderers/VideoRenderer.hpp"
#include "renderers/WallpaperEngineRenderer.hpp"
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>
namespace bwp::wallpaper {
WallpaperWindow::WallpaperWindow(const monitor::MonitorInfo &monitor)
    : m_monitor(monitor) {
  m_window = gtk_window_new();
  gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
  gtk_window_set_title(GTK_WINDOW(m_window), "BetterWallpaper-Renderer");
  gtk_layer_init_for_window(GTK_WINDOW(m_window));
  gtk_layer_set_namespace(GTK_WINDOW(m_window), "betterwallpaper");
  gtk_layer_set_layer(GTK_WINDOW(m_window), GTK_LAYER_SHELL_LAYER_BACKGROUND);
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_exclusive_zone(GTK_WINDOW(m_window), -1);
  GdkDisplay *display = gdk_display_get_default();
  GListModel *monitors = gdk_display_get_monitors(display);
  for (guint i = 0; i < g_list_model_get_n_items(monitors); i++) {
    GdkMonitor *gdkMonitor = GDK_MONITOR(g_list_model_get_item(monitors, i));
    const char *connector = gdk_monitor_get_connector(gdkMonitor);
    if (connector && std::string(connector) == monitor.name) {
      gtk_layer_set_monitor(GTK_WINDOW(m_window), gdkMonitor);
      break;
    }
  }
  m_drawingArea = gtk_drawing_area_new();
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(m_drawingArea), onDraw, this,
                                 nullptr);
  gtk_window_set_child(GTK_WINDOW(m_window), m_drawingArea);
  gtk_widget_add_tick_callback(m_drawingArea, onExtractFrame, this, nullptr);
}
WallpaperWindow::~WallpaperWindow() {
  if (m_window) {
    gtk_window_destroy(GTK_WINDOW(m_window));
  }
}
void WallpaperWindow::setRenderer(std::weak_ptr<WallpaperRenderer> renderer) {
  m_renderer = renderer;
  if (m_drawingArea)
    gtk_widget_queue_draw(m_drawingArea);
  // When the active wallpaper is WE (external process), hide our window so the
  // linux-wallpaperengine window is visible. Use a delay so the WE process can
  // create its window first. When the active wallpaper is not WE, ensure our
  // window is shown (e.g. for images, videos, or WE Video that uses VideoRenderer).
  if (auto r = m_renderer.lock()) {
    if (dynamic_cast<WallpaperEngineRenderer *>(r.get())) {
      g_timeout_add(800, onHideForWallpaperEngine, this);
    } else {
      show();
    }
  }
}
void WallpaperWindow::show() {
  if (gtk_layer_is_layer_window(GTK_WINDOW(m_window))) {
    gtk_widget_set_visible(m_window, TRUE);
  } else {
    LOG_ERROR("Window is not a layer surface! strict layer shell compliance "
              "enabled. Hiding window.");
  }
}
void WallpaperWindow::hide() { gtk_widget_set_visible(m_window, FALSE); }
void WallpaperWindow::raiseToTopForTransition() {
  // Set layer to TOP before show so we're on top when we appear (same for all types).
  if (m_window && gtk_layer_is_layer_window(GTK_WINDOW(m_window)))
    gtk_layer_set_layer(GTK_WINDOW(m_window), GTK_LAYER_SHELL_LAYER_TOP);
  show();
}
void WallpaperWindow::startTransitionWithSurfaces(
    cairo_surface_t *from, cairo_surface_t *to,
    std::shared_ptr<WallpaperRenderer> nextRenderer,
    std::function<void()> onComplete) {
  show();
  auto &conf = config::ConfigManager::getInstance();
  long durationMs = conf.get<int>("transitions.duration_ms", 500);
  std::string effectName = conf.get<std::string>("transitions.default_effect", "Fade");
  std::string easingName = conf.get<std::string>("transitions.easing", "easeInOut");
  auto effect = bwp::transition::createEffectByName(effectName);
  bool nextIsWE = nextRenderer && dynamic_cast<WallpaperEngineRenderer *>(nextRenderer.get());

  // Stay at BACKGROUND for ALL transitions. Never raise to TOP — it causes
  // a visible flash when Hyprland remaps the surface to a different layer.

  auto finish = [this, nextRenderer, onComplete, nextIsWE]() {
    LOG_INFO("Transition finished, nextIsWE=" + std::to_string(nextIsWE) +
             " frames=" + std::to_string(m_transitionFrameCount));
    m_transitionFrameCount = 0;
    if (nextIsWE) {
      // WE target: the external process has its own BACKGROUND surface.
      // We faded to black — hide our window so the WE surface is revealed.
      m_renderer = nextRenderer;
      hide();
    } else {
      // Non-WE target: bind the new renderer for normal drawing.
      setRenderer(nextRenderer);
    }
    if (onComplete) onComplete();
    if (m_drawingArea) gtk_widget_queue_draw(m_drawingArea);
  };
  LOG_INFO("startTransitionWithSurfaces: nextIsWE=" + std::to_string(nextIsWE) +
           " from=" + std::to_string(from != nullptr) +
           " to=" + std::to_string(to != nullptr) +
           " effect=" + (effect ? "valid" : "null") +
           " duration=" + std::to_string(durationMs) + "ms");
  if (!to && from && nextRenderer) {
    int w = cairo_image_surface_get_width(from);
    int h = cairo_image_surface_get_height(from);
    if (w > 0 && h > 0) {
      if (nextIsWE) {
        // →WE: fade from old content to opaque BLACK. After the transition,
        // the finish callback hides our window, revealing the WE surface below.
        m_transitionEngine.startWithLiveTo(
            from,
            [](cairo_t *cr, int /*w*/, int /*h*/) {
              cairo_set_source_rgb(cr, 0, 0, 0);
              cairo_paint(cr);
            },
            w, h, effect, durationMs, easingName, finish);
      } else {
        // →non-WE: live-render new content each frame. Start playing now.
        nextRenderer->play();
        m_transitionEngine.startWithLiveTo(
            from,
            [nextRenderer](cairo_t *cr, int width, int height) {
              if (nextRenderer) nextRenderer->render(cr, width, height);
            },
            w, h, effect, durationMs, easingName, finish);
      }
    } else {
      m_transitionEngine.start(from, nullptr, effect, durationMs, easingName, finish);
    }
  } else {
    m_transitionEngine.start(from, to, effect, durationMs, easingName, finish);
  }
  if (m_drawingArea) gtk_widget_queue_draw(m_drawingArea);
  g_timeout_add(16, onTransitionTick, this);
}

void WallpaperWindow::prepareTransitionAndStart(
    std::shared_ptr<WallpaperRenderer> nextRenderer,
    std::function<void()> onComplete) {
  if (!m_drawingArea) {
    setRenderer(nextRenderer);
    if (nextRenderer) nextRenderer->play();
    if (onComplete) onComplete();
    return;
  }
  int width = 0, height = 0;
  getTransitionDimensions(&width, &height);
  if (width <= 0 || height <= 0) {
    width = 1920;
    height = 1080;
  }
  // Build from/to while still at BACKGROUND so A stays visible (no blank).
  cairo_surface_t *from =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *crFrom = cairo_create(from);
  if (auto current = m_renderer.lock()) {
    if (dynamic_cast<WallpaperEngineRenderer *>(current.get())) {
      // Current is WE (external process) — can't capture its output.
      // Stop WE process and use opaque black for a clean black→new transition.
      current->stop();
      cairo_set_source_rgb(crFrom, 0, 0, 0);
      cairo_paint(crFrom);
    } else {
      current->render(crFrom, width, height);
    }
  } else {
    // No current renderer — black "from"
    cairo_set_source_rgb(crFrom, 0, 0, 0);
    cairo_paint(crFrom);
  }
  cairo_destroy(crFrom);
  // No "to" capture: B plays during transition; we render it live each frame.
  startTransitionWithSurfaces(from, nullptr, nextRenderer, onComplete);
  cairo_surface_destroy(from);
}
void WallpaperWindow::updateMonitor(const monitor::MonitorInfo &) {}
namespace {
struct TransitionToRetryData {
  bwp::wallpaper::WallpaperWindow *self = nullptr;
  std::shared_ptr<bwp::wallpaper::WallpaperRenderer> nextRenderer;
  std::function<void()> onComplete;
  int retryCount = 0;
};
struct TransitionPhase2Data {
  bwp::wallpaper::WallpaperWindow *self = nullptr;
  std::shared_ptr<bwp::wallpaper::WallpaperRenderer> nextRenderer;
  std::function<void()> onComplete;
  int retryCount = 0;
};

// Get effective transition size: drawing area -> m_monitor -> GdkMonitor by name -> fallback.
void getEffectiveSize(bwp::wallpaper::WallpaperWindow *self, GtkWidget *drawingArea,
                      const monitor::MonitorInfo &monitor, int *outWidth, int *outHeight) {
  int w = drawingArea ? gtk_widget_get_width(drawingArea) : 0;
  int h = drawingArea ? gtk_widget_get_height(drawingArea) : 0;
  if (w > 0 && h > 0) {
    *outWidth = w;
    *outHeight = h;
    return;
  }
  if (monitor.width > 0 && monitor.height > 0) {
    *outWidth = monitor.width;
    *outHeight = monitor.height;
    return;
  }
  GdkDisplay *display = gdk_display_get_default();
  if (display && !monitor.name.empty()) {
    GListModel *monitors = gdk_display_get_monitors(display);
    for (guint i = 0; i < g_list_model_get_n_items(monitors); i++) {
      GdkMonitor *gm = GDK_MONITOR(g_list_model_get_item(monitors, i));
      const char *connector = gdk_monitor_get_connector(gm);
      if (connector && std::string(connector) == monitor.name) {
        GdkRectangle geom;
        gdk_monitor_get_geometry(gm, &geom);
        if (geom.width > 0 && geom.height > 0) {
          *outWidth = geom.width;
          *outHeight = geom.height;
          return;
        }
        break;
      }
    }
  }
  *outWidth = 1920;
  *outHeight = 1080;
}
} // namespace

void WallpaperWindow::getTransitionDimensions(int *outWidth, int *outHeight) const {
  if (!outWidth || !outHeight) return;
  getEffectiveSize(const_cast<WallpaperWindow *>(this), m_drawingArea, m_monitor,
                   outWidth, outHeight);
  if (*outWidth <= 0 || *outHeight <= 0) {
    *outWidth = 1920;
    *outHeight = 1080;
  }
}

void WallpaperWindow::transitionTo(
    std::shared_ptr<WallpaperRenderer> nextRenderer,
    std::function<void()> onComplete, bool isRetry) {
  transitionTo(nextRenderer, onComplete, false, 0);
}
void WallpaperWindow::transitionTo(
    std::shared_ptr<WallpaperRenderer> nextRenderer,
    std::function<void()> onComplete, bool isRetry, int retryCount) {
  // Stay at BACKGROUND for all transitions. Raising to TOP causes a visible
  // flash on Hyprland when the compositor remaps the surface to a new layer.
  show();
  if (!m_drawingArea) {
    LOG_INFO("No drawing area, setting renderer directly");
    setRenderer(nextRenderer);
    if (nextRenderer)
      nextRenderer->play();
    if (onComplete)
      onComplete();
    return;
  }
  // Run capture + engine start on next idle so compositor has applied show+raise.
  // WE→WE worked because window was hidden, we show+raise, next frame we're on top.
  // For image/video we were at BACKGROUND; without idle we could draw before raise took effect.
  auto *data = new TransitionPhase2Data{this, nextRenderer, onComplete, retryCount};
  g_idle_add(onTransitionToPhase2Idle, data);
}

gboolean WallpaperWindow::onTransitionToPhase2Idle(gpointer user_data) {
  auto *d = static_cast<TransitionPhase2Data *>(user_data);
  if (d->self && d->nextRenderer)
    d->self->transitionToPhase2(d->nextRenderer, d->onComplete, d->retryCount);
  delete d;
  return G_SOURCE_REMOVE;
}

void WallpaperWindow::transitionToPhase2(
    std::shared_ptr<WallpaperRenderer> nextRenderer,
    std::function<void()> onComplete, int retryCount) {
  int width = 0, height = 0;
  getEffectiveSize(this, m_drawingArea, m_monitor, &width, &height);
  if (width <= 0 || height <= 0) {
    width = 1920;
    height = 1080;
  }
  if (gtk_widget_get_width(m_drawingArea) <= 0 || gtk_widget_get_height(m_drawingArea) <= 0) {
    if (retryCount < 3) {
      auto *data = new TransitionToRetryData{this, nextRenderer, onComplete, retryCount + 1};
      g_timeout_add(100, onTransitionToRetry, data);
      LOG_INFO("No dimensions yet, will retry transition in 100ms (attempt " +
               std::to_string(retryCount + 1) + "/3)");
      return;
    }
    LOG_INFO("Using fallback size " + std::to_string(width) + "x" + std::to_string(height) +
             " so transition always runs");
  }
  auto &conf = config::ConfigManager::getInstance();
  long durationMs = conf.get<int>("transitions.duration_ms", 500);
  std::string effectName =
      conf.get<std::string>("transitions.default_effect", "Fade");
  std::string easingName =
      conf.get<std::string>("transitions.easing", "easeInOut");
  LOG_INFO("Transitioning to new wallpaper (" + std::to_string(width) + "x" +
           std::to_string(height) + ") effect=" + effectName +
           " duration=" + std::to_string(durationMs) + "ms easing=" + easingName);
  bool nextIsWE = nextRenderer && dynamic_cast<WallpaperEngineRenderer *>(nextRenderer.get());
  cairo_surface_t *from =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *crFrom = cairo_create(from);
  if (auto current = m_renderer.lock()) {
    if (dynamic_cast<WallpaperEngineRenderer *>(current.get())) {
      // Current wallpaper is WE (external process). We can't capture its
      // rendered output. Stop the WE process immediately so our window
      // becomes the visible BACKGROUND surface, then use black as "from".
      current->stop();
      cairo_set_source_rgb(crFrom, 0, 0, 0);
      cairo_paint(crFrom);
    } else {
      current->render(crFrom, width, height);
    }
  } else {
    // No current renderer — use black as "from"
    cairo_set_source_rgb(crFrom, 0, 0, 0);
    cairo_paint(crFrom);
  }
  cairo_destroy(crFrom);

  // Diagnostic: verify from surface was captured
  LOG_INFO("From surface: " + std::to_string(cairo_image_surface_get_width(from)) +
           "x" + std::to_string(cairo_image_surface_get_height(from)) +
           " status=" + std::to_string(cairo_surface_status(from)) +
           " nextIsWE=" + std::to_string(nextIsWE) +
           " hadCurrentRenderer=" + std::to_string(m_renderer.lock() != nullptr));

  // No "to" capture: B plays during transition; we render it live each frame.
  // play() for nextRenderer is called inside startTransitionWithSurfaces.
  startTransitionWithSurfaces(from, nullptr, nextRenderer, onComplete);
  cairo_surface_destroy(from);
}
gboolean WallpaperWindow::onLowerLayerToBackground(gpointer user_data) {
  auto *self = static_cast<WallpaperWindow *>(user_data);
  if (self->m_window && gtk_layer_is_layer_window(GTK_WINDOW(self->m_window))) {
    gtk_layer_set_layer(GTK_WINDOW(self->m_window),
                        GTK_LAYER_SHELL_LAYER_BACKGROUND);
    if (self->m_drawingArea)
      gtk_widget_queue_draw(self->m_drawingArea);
  }
  return G_SOURCE_REMOVE;
}
gboolean WallpaperWindow::onHideForWallpaperEngine(gpointer user_data) {
  auto *self = static_cast<WallpaperWindow *>(user_data);
  if (self->m_window) {
    auto r = self->m_renderer.lock();
    if (r && dynamic_cast<WallpaperEngineRenderer *>(r.get()))
      self->hide();
  }
  return G_SOURCE_REMOVE;
}
gboolean WallpaperWindow::onTransitionToRetry(gpointer user_data) {
  auto *data = static_cast<TransitionToRetryData *>(user_data);
  if (data->self && data->nextRenderer)
    data->self->transitionTo(data->nextRenderer, data->onComplete, true,
                            data->retryCount);
  delete data;
  return G_SOURCE_REMOVE;
}
gboolean WallpaperWindow::onTransitionTick(gpointer user_data) {
  auto *self = static_cast<WallpaperWindow *>(user_data);
  if (!self->m_drawingArea)
    return G_SOURCE_REMOVE;
  if (self->m_transitionEngine.isActive()) {
    gtk_widget_queue_draw(self->m_drawingArea);
    return G_SOURCE_CONTINUE;
  }
  return G_SOURCE_REMOVE;
}
gboolean WallpaperWindow::onExtractFrame(GtkWidget *widget, GdkFrameClock *,
                                         gpointer user_data) {
  auto *self = static_cast<WallpaperWindow *>(user_data);
  if (self->m_transitionEngine.isActive()) {
    gtk_widget_queue_draw(widget);
    return G_SOURCE_CONTINUE;
  }
  static auto lastLog = std::chrono::steady_clock::now();
  static int frameCount = 0;
  frameCount++;
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count() >=
      5) {
    double fps = frameCount / 5.0;
    LOG_DEBUG("Render FPS: " + std::to_string(fps));
    frameCount = 0;
    lastLog = now;
  }
  if (auto renderer = self->m_renderer.lock()) {
    if (renderer->isPlaying()) {
      gtk_widget_queue_draw(widget);
      return G_SOURCE_CONTINUE;
    }
  }
  return G_SOURCE_CONTINUE;
}
void WallpaperWindow::onDraw(GtkDrawingArea *area, cairo_t *cr, int width,
                             int height, gpointer user_data) {
  auto *self = static_cast<WallpaperWindow *>(user_data);

  if (self->m_opacity < 1.0) {
    cairo_push_group(cr);
  }
  if (self->m_transitionEngine.isActive()) {
    self->m_transitionFrameCount++;
    if (!self->m_transitionEngine.render(cr, width, height)) {
      // Transition finished — callback already invoked inside render()
      gtk_widget_queue_draw(GTK_WIDGET(area));
    }
  } else if (auto renderer = self->m_renderer.lock()) {
    renderer->render(cr, width, height);
  } else {
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);
  }
  if (self->m_opacity < 1.0) {
    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, self->m_opacity);
  }
}
void WallpaperWindow::setOpacity(double opacity) {
  m_opacity = std::max(0.0, std::min(1.0, opacity));
  if (m_drawingArea) {
    gtk_widget_queue_draw(m_drawingArea);
  }
}
double WallpaperWindow::getOpacity() const { return m_opacity; }
std::shared_ptr<WallpaperRenderer> WallpaperWindow::getRenderer() const {
  return m_renderer.lock();
}
} // namespace bwp::wallpaper
