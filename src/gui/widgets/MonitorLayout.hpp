#pragma once
#include "../../core/monitor/MonitorInfo.hpp"
#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <vector>
namespace bwp::gui {
class MonitorLayout {
public:
  MonitorLayout();
  ~MonitorLayout();
  GtkWidget *getWidget() const { return m_drawingArea; }
  void setMonitors(const std::vector<bwp::monitor::MonitorInfo> &monitors);
  void selectMonitor(const std::string &name);
  const std::string &getSelectedMonitor() const { return m_selectedMonitor; }
  using SelectionCallback = std::function<void(const std::string &)>;
  void setCallback(SelectionCallback callback);
private:
  static void onDraw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
                     gpointer user_data);
  static void onClick(GtkGestureClick *gesture, int n_press, double x, double y,
                      gpointer user_data);
  GtkWidget *m_drawingArea;
  std::vector<bwp::monitor::MonitorInfo> m_monitors;
  std::string m_selectedMonitor;
  SelectionCallback m_callback;
  std::string getMonitorAt(double x, double y, int width, int height);
};
}  
