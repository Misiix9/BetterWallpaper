#include "MonitorsView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/utils/Logger.hpp"
#include <iostream>

namespace bwp::gui {

MonitorsView::MonitorsView() {
  setupUi();

  // Initial load
  refresh();

  // Load saved mode from config
  auto &config = bwp::config::ConfigManager::getInstance();
  int savedMode = config.get<int>("monitors.mode");
  if (savedMode >= 0 && savedMode <= 2) {
    m_currentMode = static_cast<MonitorMode>(savedMode);
    if (m_modeSelector) {
      adw_combo_row_set_selected(ADW_COMBO_ROW(m_modeSelector),
                                 static_cast<guint>(m_currentMode));
    }
  }
}

MonitorsView::~MonitorsView() {}

void MonitorsView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(m_box, TRUE);
  gtk_widget_set_vexpand(m_box, TRUE);

  // Mode Selector Header
  GtkWidget *modeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(modeBox, 16);
  gtk_widget_set_margin_end(modeBox, 16);
  gtk_widget_set_margin_top(modeBox, 12);
  gtk_widget_set_margin_bottom(modeBox, 8);

  GtkWidget *modeLabel = gtk_label_new("Multi-Monitor Mode");
  gtk_widget_add_css_class(modeLabel, "title-4");
  gtk_widget_set_halign(modeLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(modeLabel, TRUE);
  gtk_box_append(GTK_BOX(modeBox), modeLabel);

  // Mode dropdown using AdwComboRow style dropdown
  const char *modes[] = {"Independent", "Clone", "Span", nullptr};
  GtkStringList *modeList = gtk_string_list_new(modes);

  m_modeSelector = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_modeSelector),
                                "Display Mode");
  adw_combo_row_set_model(ADW_COMBO_ROW(m_modeSelector),
                          G_LIST_MODEL(modeList));
  g_object_unref(modeList);

  // Subtitles for each mode
  adw_action_row_set_subtitle(
      ADW_ACTION_ROW(m_modeSelector),
      "Independent: Each monitor has its own wallpaper");

  g_signal_connect(
      m_modeSelector, "notify::selected",
      G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer data) {
        auto *self = static_cast<MonitorsView *>(data);
        guint selected = adw_combo_row_get_selected(ADW_COMBO_ROW(object));
        MonitorMode newMode = static_cast<MonitorMode>(selected);
        self->onModeChanged(newMode);
      }),
      this);

  // Wrap in a preferences group for nice styling
  GtkWidget *modeGroup = adw_preferences_group_new();
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(modeGroup), m_modeSelector);
  gtk_widget_set_margin_start(modeGroup, 12);
  gtk_widget_set_margin_end(modeGroup, 12);
  gtk_widget_set_margin_top(modeGroup, 12);
  gtk_box_append(GTK_BOX(m_box), modeGroup);

  // Main Content Area (Split View)
  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(paned), 350); // 350px for layout
  gtk_box_append(GTK_BOX(m_box), paned);

  // Top: Monitor Layout
  m_layout = std::make_unique<MonitorLayout>();
  gtk_paned_set_start_child(GTK_PANED(paned), m_layout->getWidget());
  gtk_paned_set_resize_start_child(GTK_PANED(paned), TRUE);
  gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);

  // Bottom: Configuration
  m_configPanel = std::make_unique<MonitorConfigPanel>();
  GtkWidget *configWidget = m_configPanel->getWidget();
  gtk_widget_set_vexpand(configWidget, FALSE);

  // Wrap config in a scrolled window just in case
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), configWidget);
  gtk_paned_set_end_child(GTK_PANED(paned), scrolled);
  gtk_paned_set_resize_end_child(GTK_PANED(paned), FALSE);
  gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);

  // Connect Selection
  m_layout->setCallback([this](const std::string &name) {
    auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();
    for (const auto &mon : monitors) {
      if (mon.name == name) {
        m_configPanel->setMonitor(mon);
        break;
      }
    }
  });

  // Connect Configuration Changes (Change Wallpaper)
  m_configPanel->setCallback([this](const std::string &monitorName) {
    // Open file chooser to pick wallpaper for this monitor
    auto dialog = gtk_file_chooser_native_new(
        "Select Wallpaper", GTK_WINDOW(gtk_widget_get_root(m_box)),
        GTK_FILE_CHOOSER_ACTION_OPEN, "_Select", "_Cancel");

    // Pass monitor name as data
    std::string *monitorNamePtr = new std::string(monitorName);
    g_object_set_data_full(
        G_OBJECT(dialog), "monitor_name", monitorNamePtr,
        [](gpointer data) { delete static_cast<std::string *>(data); });

    g_signal_connect(
        dialog, "response",
        G_CALLBACK(+[](GtkNativeDialog *dialog, int response, gpointer) {
          if (response == GTK_RESPONSE_ACCEPT) {
            GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
            char *path = g_file_get_path(file);

            std::string *mon = static_cast<std::string *>(
                g_object_get_data(G_OBJECT(dialog), "monitor_name"));

            if (path && mon) {
              bwp::ipc::DBusClient client;
              if (client.connect()) {
                client.setWallpaper(path, *mon);
              } else {
                std::cerr << "Failed to connect to daemon\n";
              }
              g_free(path);
            }
            g_object_unref(file);
          }
          g_object_unref(dialog);
        }),
        nullptr);

    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
  });
}

void MonitorsView::refresh() {
  auto &manager = bwp::monitor::MonitorManager::getInstance();
  if (!manager.initialize()) {
    std::cerr << "Failed to initialize MonitorManager" << std::endl;
    return;
  }

  // Force update to get latest state (non-blocking)
  manager.processPending();

  auto monitors = manager.getMonitors();
  m_layout->setMonitors(monitors);

  // If nothing selected, clear config
  if (m_layout->getSelectedMonitor().empty()) {
    m_configPanel->clear();

    // Auto-select primary or first if available
    if (!monitors.empty()) {
      m_layout->selectMonitor(monitors[0].name);
    }
  }
}

void MonitorsView::setMonitorMode(MonitorMode mode) {
  if (m_currentMode == mode)
    return;

  m_currentMode = mode;

  // Update UI
  if (m_modeSelector) {
    adw_combo_row_set_selected(ADW_COMBO_ROW(m_modeSelector),
                               static_cast<guint>(mode));
  }

  // Save to config
  auto &config = bwp::config::ConfigManager::getInstance();
  config.set<int>("monitors.mode", static_cast<int>(mode));

  LOG_INFO("Monitor mode changed to: " + std::to_string(static_cast<int>(mode)));
}

void MonitorsView::onModeChanged(MonitorMode mode) {
  if (m_currentMode == mode)
    return;

  m_currentMode = mode;

  // Save to config
  auto &config = bwp::config::ConfigManager::getInstance();
  config.set<int>("monitors.mode", static_cast<int>(mode));

  // Update subtitle based on mode
  const char *subtitle = "";
  switch (mode) {
  case MonitorMode::Independent:
    subtitle = "Each monitor has its own wallpaper";
    break;
  case MonitorMode::Clone:
    subtitle = "Same wallpaper on all monitors (scaled)";
    break;
  case MonitorMode::Span:
    subtitle = "Single wallpaper stretched across all monitors";
    break;
  }
  adw_action_row_set_subtitle(ADW_ACTION_ROW(m_modeSelector), subtitle);

  // Enable/disable per-monitor config based on mode
  if (mode == MonitorMode::Independent) {
    // Full config panel enabled
    m_configPanel->getWidget();
    gtk_widget_set_sensitive(m_configPanel->getWidget(), TRUE);
  } else {
    // In Clone/Span modes, per-monitor config is limited
    // The config panel still works but affects all monitors
  }

  LOG_INFO("Monitor mode changed to: " + std::to_string(static_cast<int>(mode)));
}

void MonitorsView::applyCloneMode(const std::string &wallpaperPath) {
  if (wallpaperPath.empty())
    return;

  // Apply the same wallpaper to all monitors
  bwp::ipc::DBusClient client;
  if (!client.connect()) {
    LOG_ERROR("Failed to connect to daemon for Clone mode");
    return;
  }

  auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();
  for (const auto &monitor : monitors) {
    client.setWallpaper(wallpaperPath, monitor.name);
    LOG_INFO("Clone mode: Set wallpaper on " + monitor.name);
  }
}

void MonitorsView::applySpanMode(const std::string &wallpaperPath) {
  if (wallpaperPath.empty())
    return;

  // For Span mode, we need to calculate the combined display area
  // and set the wallpaper with appropriate offsets for each monitor
  auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();

  if (monitors.empty())
    return;

  // Calculate bounding box of all monitors
  int minX = 0, minY = 0, maxX = 0, maxY = 0;
  for (const auto &mon : monitors) {
    if (mon.x < minX)
      minX = mon.x;
    if (mon.y < minY)
      minY = mon.y;
    if (mon.x + mon.width > maxX)
      maxX = mon.x + mon.width;
    if (mon.y + mon.height > maxY)
      maxY = mon.y + mon.height;
  }

  int totalWidth = maxX - minX;
  int totalHeight = maxY - minY;

  LOG_INFO("Span mode: Combined resolution " + std::to_string(totalWidth) +
           "x" + std::to_string(totalHeight));

  // For Span mode, we need the daemon to support "span" scaling mode
  // with offset parameters. For now, set wallpaper with span info.
  // This requires daemon support to properly crop/position the wallpaper
  // based on each monitor's position in the combined space.

  bwp::ipc::DBusClient client;
  if (!client.connect()) {
    LOG_ERROR("Failed to connect to daemon for Span mode");
    return;
  }

  // Set the wallpaper with span mode indicator
  // The daemon would need to handle this specially
  for (const auto &monitor : monitors) {
    // Calculate offset for this monitor in the combined space
    double offsetX = static_cast<double>(monitor.x - minX) / totalWidth;
    double offsetY = static_cast<double>(monitor.y - minY) / totalHeight;
    double scaleX = static_cast<double>(monitor.width) / totalWidth;
    double scaleY = static_cast<double>(monitor.height) / totalHeight;

    // For now, just set the wallpaper - proper span rendering
    // would need daemon-side support with these parameters
    client.setWallpaper(wallpaperPath, monitor.name);

    LOG_INFO("Span mode: " + monitor.name + " offset(" +
             std::to_string(offsetX) + ", " + std::to_string(offsetY) +
             ") scale(" + std::to_string(scaleX) + ", " +
             std::to_string(scaleY) + ")");
  }
}

void MonitorsView::updateMonitorList() { refresh(); }

} // namespace bwp::gui
