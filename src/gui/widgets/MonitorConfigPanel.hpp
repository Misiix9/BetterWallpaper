#pragma once

#include "../../core/monitor/MonitorInfo.hpp"
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <string>

namespace bwp::gui {

class MonitorConfigPanel {
public:
  MonitorConfigPanel();
  ~MonitorConfigPanel();

  GtkWidget *getWidget() const { return m_box; }

  // Update the panel with details of the selected monitor
  void setMonitor(const bwp::monitor::MonitorInfo &info);

  // Clear the panel (when no monitor is selected)
  void clear();

  using ChangeWallpaperCallback =
      std::function<void(const std::string &)>; // returns monitor name
  void setCallback(ChangeWallpaperCallback callback);

private:
  void setupUi();

  GtkWidget *m_box;
  GtkWidget *m_nameLabel;
  GtkWidget *m_resLabel;
  GtkWidget *m_scaleLabel;
  GtkWidget *m_wallpaperLabel;
  GtkWidget *m_changeButton;
  GtkWidget *m_modeDropdown;

  std::string m_currentMonitorName;
  ChangeWallpaperCallback m_callback;
};

} // namespace bwp::gui
