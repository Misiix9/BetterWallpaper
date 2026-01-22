#pragma once
#include "../../core/monitor/MonitorInfo.hpp"
#include "../../core/wallpaper/WallpaperInfo.hpp"
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
  void setWallpaperInfo(const bwp::wallpaper::WallpaperInfo &info);

private:
  GtkWidget *m_box;
  GtkWidget *m_titleLabel;
  GtkWidget *m_specsLabel;

  // Controls
  GtkWidget *m_scalingRow; // AdwComboRow
  GtkWidget *m_volumeRow;  // AdwActionRow with scale
  GtkWidget *m_speedRow;   // AdwActionRow with scale

  bwp::monitor::MonitorInfo m_currentMonitor;
};

} // namespace bwp::gui
