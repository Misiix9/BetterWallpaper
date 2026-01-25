#include "WallpaperWindow.hpp"
#include "../transition/effects/BasicEffects.hpp"
#include "../utils/Logger.hpp"
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h> // Assuming this library is used

namespace bwp::wallpaper {

WallpaperWindow::WallpaperWindow(const monitor::MonitorInfo &monitor)
    : m_monitor(monitor) {
  m_window = gtk_window_new();

  // Initialize Layer Shell
  gtk_layer_init_for_window(GTK_WINDOW(m_window));

  // Set parameters
  gtk_layer_set_layer(GTK_WINDOW(m_window), GTK_LAYER_SHELL_LAYER_BACKGROUND);

  // Anchor to all edges
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(m_window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);

  // Exclusive zone -1 to ignore
  gtk_layer_set_exclusive_zone(GTK_WINDOW(m_window), -1);

  // Assign to monitor
  GdkDisplay *display = gdk_display_get_default();
  // Getting GdkMonitor from monitor name is tricky, iterating display monitors:
  GListModel *monitors = gdk_display_get_monitors(display);
  for (guint i = 0; i < g_list_model_get_n_items(monitors); i++) {
    GdkMonitor *gdkMonitor = GDK_MONITOR(g_list_model_get_item(monitors, i));
    const char *model =
        gdk_monitor_get_model(gdkMonitor); // Not name, but might match
    const char *connector = gdk_monitor_get_connector(gdkMonitor);

    if (connector && std::string(connector) == monitor.name) {
      gtk_layer_set_monitor(GTK_WINDOW(m_window), gdkMonitor);
      break;
    }
  }

  // Setup drawing area
  m_drawingArea = gtk_drawing_area_new();
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(m_drawingArea), onDraw, this,
                                 nullptr);
  gtk_window_set_child(GTK_WINDOW(m_window), m_drawingArea);

  // Frame clock for animations
  gtk_widget_add_tick_callback(m_drawingArea, onExtractFrame, this, nullptr);
}

WallpaperWindow::~WallpaperWindow() {
  if (m_window) {
    gtk_window_destroy(GTK_WINDOW(m_window));
  }
}

void WallpaperWindow::setRenderer(std::weak_ptr<WallpaperRenderer> renderer) {
  m_renderer = renderer;
  gtk_widget_queue_draw(m_drawingArea);
}

void WallpaperWindow::show() { gtk_widget_show(m_window); }

void WallpaperWindow::hide() { gtk_widget_hide(m_window); }

void WallpaperWindow::updateMonitor(const monitor::MonitorInfo &monitor) {
  // Re-assign monitor logic if changed
}

void WallpaperWindow::transitionTo(
    std::shared_ptr<WallpaperRenderer> nextRenderer) {
  if (!m_drawingArea || m_renderer.expired()) {
    setRenderer(nextRenderer);
    return;
  }

  int width = gtk_widget_get_width(m_drawingArea);
  int height = gtk_widget_get_height(m_drawingArea);

  // Snapshots
  cairo_surface_t *from =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *crFrom = cairo_create(from);
  if (auto current = m_renderer.lock()) {
    current->render(crFrom, width, height);
  }
  cairo_destroy(crFrom);

  cairo_surface_t *to =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *crTo = cairo_create(to);
  if (nextRenderer) {
    nextRenderer->render(crTo, width, height);
  }
  cairo_destroy(crTo);

  // Start transition (Fade default)
  auto effect = std::make_shared<bwp::transition::FadeEffect>();
  m_transitionEngine.start(from, to, effect, 600, [this, nextRenderer]() {
    setRenderer(nextRenderer);
    if (nextRenderer)
      nextRenderer->play();
  });

  cairo_surface_destroy(from);
  cairo_surface_destroy(to);

  gtk_widget_queue_draw(m_drawingArea);
}

gboolean WallpaperWindow::onExtractFrame(GtkWidget *widget,
                                         GdkFrameClock *clock,
                                         gpointer user_data) {
  auto *self = static_cast<WallpaperWindow *>(user_data);

  if (self->m_transitionEngine.isActive()) {
    gtk_widget_queue_draw(widget);
    return G_SOURCE_CONTINUE;
  }

  // If renderer requires animation loop, queue draw
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

  if (self->m_transitionEngine.isActive()) {
    if (!self->m_transitionEngine.render(cr, width, height)) {
      // Finished just now
      gtk_widget_queue_draw(GTK_WIDGET(area));
    }
    return;
  }

  if (auto renderer = self->m_renderer.lock()) {
    renderer->render(cr, width, height); // Corrected argument count
  } else {
    // Black background default
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);
  }
}

} // namespace bwp::wallpaper
