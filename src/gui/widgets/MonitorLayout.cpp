#include "MonitorLayout.hpp"
#include "../../core/config/ConfigManager.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
namespace bwp::gui {
MonitorLayout::MonitorLayout() {
  m_drawingArea = gtk_drawing_area_new();
  gtk_widget_set_hexpand(m_drawingArea, TRUE);
  gtk_widget_set_vexpand(m_drawingArea, TRUE);
  gtk_widget_set_size_request(m_drawingArea, -1, 200);  
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(m_drawingArea), onDraw, this,
                                 nullptr);
  GtkGesture *click = gtk_gesture_click_new();
  g_signal_connect(click, "pressed", G_CALLBACK(onClick), this);
  gtk_widget_add_controller(m_drawingArea, GTK_EVENT_CONTROLLER(click));
}
MonitorLayout::~MonitorLayout() {}
void MonitorLayout::setMonitors(
    const std::vector<bwp::monitor::MonitorInfo> &monitors) {
  m_monitors = monitors;
  gtk_widget_queue_draw(m_drawingArea);
}
void MonitorLayout::selectMonitor(const std::string &name) {
  if (m_selectedMonitor != name) {
    m_selectedMonitor = name;
    gtk_widget_queue_draw(m_drawingArea);
    if (m_callback) {
      m_callback(name);
    }
  }
}
void MonitorLayout::setCallback(SelectionCallback callback) {
  m_callback = callback;
}
void MonitorLayout::onDraw(GtkDrawingArea *area, cairo_t *cr, int width,
                           int height, gpointer user_data) {
  auto *self = static_cast<MonitorLayout *>(user_data);
  if (self->m_monitors.empty()) {
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16);
    cairo_move_to(cr, width / 2.0 - 50, height / 2.0);
    cairo_show_text(cr, "No Monitors");
    return;
  }
  int minX = 0, minY = 0, maxX = 0, maxY = 0;
  bool first = true;
  for (const auto &mon : self->m_monitors) {
    int mx = mon.x;
    int my = mon.y;
    int mw = mon.logicalWidth();
    int mh = mon.logicalHeight();
    if (first) {
      minX = mx;
      minY = my;
      maxX = mx + mw;
      maxY = my + mh;
      first = false;
    } else {
      minX = std::min(minX, mx);
      minY = std::min(minY, my);
      maxX = std::max(maxX, mx + mw);
      maxY = std::max(maxY, my + mh);
    }
  }
  double totalW = maxX - minX;
  double totalH = maxY - minY;
  double padding = 20.0;
  double availableW = width - (padding * 2);
  double availableH = height - (padding * 2);
  double scaleX = availableW / totalW;
  double scaleY = availableH / totalH;
  double scale = std::min(scaleX, scaleY);
  double drawnW = totalW * scale;
  double drawnH = totalH * scale;
  double offsetX = (width - drawnW) / 2.0;
  double offsetY = (height - drawnH) / 2.0;
  for (const auto &mon : self->m_monitors) {
    double rx = offsetX + (mon.x - minX) * scale;
    double ry = offsetY + (mon.y - minY) * scale;
    double rw = mon.logicalWidth() * scale;
    double rh = mon.logicalHeight() * scale;
    bool isSelected = (mon.name == self->m_selectedMonitor);
    if (isSelected) {
      cairo_set_source_rgb(cr, 0.2, 0.4, 0.8);  
    } else {
      cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);  
    }
    cairo_rectangle(cr, rx, ry, rw, rh);
    cairo_fill_preserve(cr);
    cairo_set_line_width(cr, isSelected ? 3.0 : 1.0);
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);  
    cairo_stroke(cr);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    double fontSize = std::min(14.0, rh / 4.0);  
    cairo_set_font_size(cr, fontSize);
    std::string customKey = "monitors." + mon.name + ".custom_name";
    auto &config = bwp::config::ConfigManager::getInstance();
    std::string customName = config.get<std::string>(customKey, "");
    std::string displayName =
        customName.empty() ? mon.name : customName;
    cairo_text_extents_t extents;
    cairo_text_extents(cr, displayName.c_str(), &extents);
    if (!customName.empty()) {
      double nameY = ry + (rh / 2.0) - 2.0;
      cairo_move_to(cr, rx + (rw - extents.width) / 2.0,
                    nameY);
      cairo_show_text(cr, displayName.c_str());
      double subFontSize = std::max(8.0, fontSize * 0.65);
      cairo_set_font_size(cr, subFontSize);
      cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
      cairo_text_extents_t subExtents;
      cairo_text_extents(cr, mon.name.c_str(), &subExtents);
      cairo_move_to(cr, rx + (rw - subExtents.width) / 2.0,
                    nameY + subFontSize + 4.0);
      cairo_show_text(cr, mon.name.c_str());
    } else {
      cairo_move_to(cr, rx + (rw - extents.width) / 2.0,
                    ry + (rh + extents.height) / 2.0);
      cairo_show_text(cr, displayName.c_str());
    }
  }
}
void MonitorLayout::onClick(GtkGestureClick *gesture, int n_press, double x,
                            double y, gpointer user_data) {
  auto *self = static_cast<MonitorLayout *>(user_data);
  int width = gtk_widget_get_width(self->m_drawingArea);
  int height = gtk_widget_get_height(self->m_drawingArea);
  std::string clickedMon = self->getMonitorAt(x, y, width, height);
  if (!clickedMon.empty()) {
    self->selectMonitor(clickedMon);
  }
}
std::string MonitorLayout::getMonitorAt(double x, double y, int width,
                                        int height) {
  if (m_monitors.empty())
    return "";
  int minX = 0, minY = 0, maxX = 0, maxY = 0;
  bool first = true;
  for (const auto &mon : m_monitors) {
    int mx = mon.x;
    int my = mon.y;
    int mw = mon.logicalWidth();
    int mh = mon.logicalHeight();
    if (first) {
      minX = mx;
      minY = my;
      maxX = mx + mw;
      maxY = my + mh;
      first = false;
    } else {
      minX = std::min(minX, mx);
      minY = std::min(minY, my);
      maxX = std::max(maxX, mx + mw);
      maxY = std::max(maxY, my + mh);
    }
  }
  double totalW = maxX - minX;
  double totalH = maxY - minY;
  double padding = 20.0;
  double availableW = width - (padding * 2);
  double availableH = height - (padding * 2);
  double scaleX = availableW / totalW;
  double scaleY = availableH / totalH;
  double scale = std::min(scaleX, scaleY);
  double drawnW = totalW * scale;
  double drawnH = totalH * scale;
  double offsetX = (width - drawnW) / 2.0;
  double offsetY = (height - drawnH) / 2.0;
  for (const auto &mon : m_monitors) {
    double rx = offsetX + (mon.x - minX) * scale;
    double ry = offsetY + (mon.y - minY) * scale;
    double rw = mon.logicalWidth() * scale;
    double rh = mon.logicalHeight() * scale;
    if (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh) {
      return mon.name;
    }
  }
  return "";
}
}  
