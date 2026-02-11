#pragma once
#include "../../core/ipc/LinuxIPCClient.hpp"
#include "../widgets/MonitorConfigPanel.hpp"
#include "../widgets/MonitorLayout.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <string>
namespace bwp::gui {
enum class MonitorMode { Independent = 0, Clone = 1, Span = 2 };
class MonitorsView {
public:
  MonitorsView();
  ~MonitorsView();
  GtkWidget *getWidget() const { return m_box; }
  void refresh();
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
  GtkWidget *m_modeStack = nullptr;  
  std::unique_ptr<MonitorLayout> m_layout;
  std::unique_ptr<MonitorConfigPanel> m_configPanel;
  MonitorMode m_currentMode = MonitorMode::Independent;
};
}  
