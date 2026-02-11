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
  void setMonitor(const bwp::monitor::MonitorInfo &info);
  void clear();
  using ChangeWallpaperCallback =
      std::function<void(const std::string &)>;  
  void setCallback(ChangeWallpaperCallback callback);
private:
  void setupUi();
  void saveCustomName();
  GtkWidget *m_box;
  GtkWidget *m_nameLabel;       
  GtkWidget *m_customNameEntry;  
  GtkWidget *m_resLabel;
  GtkWidget *m_scaleLabel;
  GtkWidget *m_wallpaperLabel;
  GtkWidget *m_changeButton;
  GtkWidget *m_modeDropdown;
  std::string m_currentMonitorName;
  ChangeWallpaperCallback m_callback;
};
}  
