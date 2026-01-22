#pragma once
#include "../widgets/MonitorConfigPanel.hpp"
#include "../widgets/MonitorLayout.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>

namespace bwp::gui {

class MonitorsView {
public:
  MonitorsView();
  ~MonitorsView();

  GtkWidget *getWidget() const { return m_mainBox; }
  void refresh();

private:
  void setupUi();
  void loadMonitors();

  GtkWidget *m_mainBox;
  std::unique_ptr<MonitorLayout> m_layout;
  std::unique_ptr<MonitorConfigPanel> m_configPanel;
};

} // namespace bwp::gui
