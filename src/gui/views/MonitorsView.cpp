#include "MonitorsView.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/wallpaper/WallpaperManager.hpp"

namespace bwp::gui {

MonitorsView::MonitorsView() {
  setupUi();
  refresh();
}

MonitorsView::~MonitorsView() {}

void MonitorsView::setupUi() {
  m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
  gtk_widget_set_margin_start(m_mainBox, 24);
  gtk_widget_set_margin_end(m_mainBox, 24);
  gtk_widget_set_margin_top(m_mainBox, 24);
  gtk_widget_set_margin_bottom(m_mainBox, 24);

  // Header
  GtkWidget *header = gtk_label_new("Monitor Configuration");
  gtk_widget_add_css_class(header, "title-1");
  gtk_widget_set_halign(header, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_mainBox), header);

  // Monitors Layout Widget
  m_layout = std::make_unique<MonitorLayout>();
  gtk_box_append(GTK_BOX(m_mainBox), m_layout->getWidget());

  m_layout->setCallback([this](const std::string &name) {
    auto monitor = bwp::monitor::MonitorManager::getInstance().getMonitor(name);
    if (monitor) {
      m_configPanel->setMonitor(*monitor);
      // Get current wallpaper info...
      std::string path =
          bwp::wallpaper::WallpaperManager::getInstance().getCurrentWallpaper(
              name);
      if (!path.empty()) {
        bwp::wallpaper::WallpaperInfo info;
        info.path = path;
        // Fill stub info
        m_configPanel->setWallpaperInfo(info);
      }
    }
  });

  gtk_box_append(GTK_BOX(m_mainBox),
                 gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  // Config Panel
  m_configPanel = std::make_unique<MonitorConfigPanel>();
  gtk_box_append(GTK_BOX(m_mainBox), m_configPanel->getWidget());
}

void MonitorsView::refresh() { loadMonitors(); }

void MonitorsView::loadMonitors() {
  auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();
  m_layout->setMonitors(monitors);
  if (!monitors.empty()) {
    m_layout->selectMonitor(monitors[0].name);
  }
}

} // namespace bwp::gui
