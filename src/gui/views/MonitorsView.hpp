#pragma once

#include "../../core/ipc/DBusClient.hpp"
#include "../widgets/MonitorConfigPanel.hpp"
#include "../widgets/MonitorLayout.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <string>

namespace bwp::gui {

// Multi-monitor modes
enum class MonitorMode { Independent = 0, Clone = 1, Span = 2 };

class MonitorsView {
public:
  MonitorsView();
  ~MonitorsView();

  GtkWidget *getWidget() const { return m_box; }

  // Refresh monitor list from Daemon/Manager
  void refresh();

  // Multi-monitor mode
  MonitorMode getMonitorMode() const { return m_currentMode; }
  void setMonitorMode(MonitorMode mode);

private:
  void setupUi();
  void updateMonitorList();
  void onModeChanged(MonitorMode mode);
  void applyCloneMode(const std::string &wallpaperPath);
  void applySpanMode(const std::string &wallpaperPath);

  GtkWidget *m_box;
  GtkWidget *m_modeSelector = nullptr;
  GtkWidget *m_modeStack = nullptr; // For switching modes (Independent/Clone/Span)

  std::unique_ptr<MonitorLayout> m_layout;
  std::unique_ptr<MonitorConfigPanel> m_configPanel;

  MonitorMode m_currentMode = MonitorMode::Independent;
};

} // namespace bwp::gui
