#include "MonitorLayout.hpp"
#include <algorithm>
#include <cmath>

namespace bwp::gui {

MonitorLayout::MonitorLayout() {
  m_drawingArea = gtk_drawing_area_new();
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(m_drawingArea), onDraw, this,
                                 nullptr);

  // Minimum size
  gtk_widget_set_size_request(m_drawingArea, 400, 200);

  // Click gesture
  GtkGesture *click = gtk_gesture_click_new();
  g_signal_connect(click, "pressed", G_CALLBACK(onClick), this);
  gtk_widget_add_controller(m_drawingArea, GTK_EVENT_CONTROLLER(click));
}

MonitorLayout::~MonitorLayout() {}

void MonitorLayout::setMonitors(
    const std::vector<bwp::monitor::MonitorInfo> &monitors) {
  m_monitors = monitors;
  if (m_selectedMonitor.empty() && !m_monitors.empty()) {
    m_selectedMonitor = m_monitors[0].name;
  }
  gtk_widget_queue_draw(m_drawingArea);
}

void MonitorLayout::selectMonitor(const std::string &name) {
  m_selectedMonitor = name;
  gtk_widget_queue_draw(m_drawingArea);
  if (m_callback)
    m_callback(name);
}

void MonitorLayout::setCallback(SelectionCallback callback) {
  m_callback = callback;
}

void MonitorLayout::onDraw(GtkDrawingArea *area, cairo_t *cr, int width,
                           int height, gpointer user_data) {
  auto *self = static_cast<MonitorLayout *>(user_data);
  if (self->m_monitors.empty())
    return;

  // Calculate bounding box
  int minX = 0, minY = 0, maxX = 0, maxY = 0;
  for (const auto &m : self->m_monitors) {
    minX = std::min(minX, m.x);
    minY = std::min(minY, m.y);
    maxX = std::max(maxX, m.x + m.width);
    maxY = std::max(maxY, m.y + m.height);
  }

  int totalW = maxX - minX;
  int totalH = maxY - minY;
  if (totalW == 0 || totalH == 0)
    return;

  // Calculate scale to fit
  double scaleX = (double)(width - 40) / totalW;
  double scaleY = (double)(height - 40) / totalH;
  double scale = std::min(scaleX, scaleY);

  // Center it
  double offsetX = (width - totalW * scale) / 2.0;
  double offsetY = (height - totalH * scale) / 2.0;

  // Draw
  cairo_set_line_width(cr, 2.0);

  for (const auto &m : self->m_monitors) {
    double x = offsetX + (m.x - minX) * scale;
    double y = offsetY + (m.y - minY) * scale;
    double w = m.width * scale;
    double h = m.height * scale;

    // Fill
    if (m.name == self->m_selectedMonitor) {
      cairo_set_source_rgb(cr, 0.2, 0.4, 0.8); // Accent color ish
    } else {
      cairo_set_source_rgb(cr, 0.3, 0.3, 0.3); // Dark gray
    }
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill_preserve(cr);

    // Stroke
    if (m.name == self->m_selectedMonitor) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    } else {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_stroke(cr);

    // Label
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, m.name.c_str(), &extents);
    cairo_move_to(cr, x + (w - extents.width) / 2,
                  y + (h + extents.height) / 2);
    cairo_show_text(cr, m.name.c_str());
  }
}

void MonitorLayout::onClick(GtkGestureClick *gesture, int n_press, double x,
                            double y, gpointer user_data) {
  auto *self = static_cast<MonitorLayout *>(user_data);
  int width = gtk_widget_get_width(self->m_drawingArea);
  int height = gtk_widget_get_height(self->m_drawingArea);

  std::string hit = self->getMonitorAt(x, y, width, height);
  if (!hit.empty() && hit != self->m_selectedMonitor) {
    self->selectMonitor(hit);
  }
}

std::string MonitorLayout::getMonitorAt(double px, double py, int width,
                                        int height) {
  if (m_monitors.empty())
    return "";

  int minX = 0, minY = 0, maxX = 0, maxY = 0;
  for (const auto &m : m_monitors) {
    minX = std::min(minX, m.x);
    minY = std::min(minY, m.y);
    maxX = std::max(maxX, m.x + m.width);
    maxY = std::max(maxY, m.y + m.height);
  }

  int totalW = maxX - minX;
  int totalH = maxY - minY;
  if (totalW == 0 || totalH == 0)
    return "";

  double scaleX = (double)(width - 40) / totalW;
  double scaleY = (double)(height - 40) / totalH;
  double scale = std::min(scaleX, scaleY);

  double offsetX = (width - totalW * scale) / 2.0;
  double offsetY = (height - totalH * scale) / 2.0;

  for (const auto &m :
       m_monitors) { // Iterate reverse to click top one? Usually no overlap.
    double x = offsetX + (m.x - minX) * scale;
    double y = offsetY + (m.y - minY) * scale;
    double w = m.width * scale;
    double h = m.height * scale;

    if (px >= x && px <= x + w && py >= y && py <= y + h) {
      return m.name;
    }
  }
  return "";
}

} // namespace bwp::gui
