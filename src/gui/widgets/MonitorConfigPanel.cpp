#include "MonitorConfigPanel.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/utils/Logger.hpp"
#include <string>
namespace bwp::gui {
MonitorConfigPanel::MonitorConfigPanel() { setupUi(); }
MonitorConfigPanel::~MonitorConfigPanel() {}
void MonitorConfigPanel::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(m_box, 12);
  gtk_widget_set_margin_end(m_box, 12);
  gtk_widget_set_margin_top(m_box, 12);
  gtk_widget_set_margin_bottom(m_box, 12);
  GtkWidget *header = gtk_label_new("Monitor Settings");
  gtk_widget_add_css_class(header, "title-4");
  gtk_widget_set_halign(header, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_box), header);
  GtkWidget *group = adw_preferences_group_new();
  gtk_box_append(GTK_BOX(m_box), group);
  GtkWidget *nameRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(nameRow), "Connector");
  m_nameLabel = gtk_label_new("-");
  gtk_widget_add_css_class(m_nameLabel, "dim-label");
  adw_action_row_add_suffix(ADW_ACTION_ROW(nameRow), m_nameLabel);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), nameRow);
  GtkWidget *customNameRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(customNameRow),
                                "Display Name");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(customNameRow),
                              "Custom label for this monitor");
  m_customNameEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(m_customNameEntry),
                                 "e.g. Main Display");
  gtk_widget_set_valign(m_customNameEntry, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request(m_customNameEntry, 160, -1);
  adw_action_row_add_suffix(ADW_ACTION_ROW(customNameRow), m_customNameEntry);
  g_signal_connect(
      m_customNameEntry, "activate",
      G_CALLBACK(+[](GtkEntry *, gpointer data) {
        static_cast<MonitorConfigPanel *>(data)->saveCustomName();
      }),
      this);
  auto *focusCtrl = gtk_event_controller_focus_new();
  g_signal_connect(
      focusCtrl, "leave",
      G_CALLBACK(+[](GtkEventControllerFocus *, gpointer data) {
        static_cast<MonitorConfigPanel *>(data)->saveCustomName();
      }),
      this);
  gtk_widget_add_controller(m_customNameEntry, focusCtrl);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), customNameRow);
  GtkWidget *resRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(resRow), "Resolution");
  m_resLabel = gtk_label_new("-");
  adw_action_row_add_suffix(ADW_ACTION_ROW(resRow), m_resLabel);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), resRow);
  GtkWidget *scaleRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scaleRow), "Scale");
  m_scaleLabel = gtk_label_new("-");
  adw_action_row_add_suffix(ADW_ACTION_ROW(scaleRow), m_scaleLabel);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), scaleRow);
  GtkWidget *modeRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(modeRow),
                                "Wallpaper Scaling");
  const char *modes[] = {"Fill", "Fit", "Stretch", "Center", "Tile", nullptr};
  GtkStringList *list = gtk_string_list_new(modes);
  adw_combo_row_set_model(ADW_COMBO_ROW(modeRow), G_LIST_MODEL(list));
  g_object_unref(list);
  m_modeDropdown = modeRow;
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), modeRow);
  m_changeButton = gtk_button_new_with_label("Change Wallpaper");
  gtk_widget_add_css_class(m_changeButton, "suggested-action");
  gtk_widget_set_margin_top(m_changeButton, 12);
  g_signal_connect(
      m_changeButton, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<MonitorConfigPanel *>(data);
        if (self->m_callback && !self->m_currentMonitorName.empty()) {
          self->m_callback(self->m_currentMonitorName);
        }
      }),
      this);
  gtk_box_append(GTK_BOX(m_box), m_changeButton);
}
void MonitorConfigPanel::saveCustomName() {
  if (m_currentMonitorName.empty())
    return;
  const char *text =
      gtk_editable_get_text(GTK_EDITABLE(m_customNameEntry));
  std::string customName = text ? text : "";
  std::string key =
      "monitors." + m_currentMonitorName + ".custom_name";
  auto &config = bwp::config::ConfigManager::getInstance();
  std::string existing = config.get<std::string>(key, "");
  if (existing != customName) {
    config.set(key, customName);
    LOG_INFO("Custom name for " + m_currentMonitorName + " set to: " +
             customName);
  }
}
void MonitorConfigPanel::setMonitor(const bwp::monitor::MonitorInfo &info) {
  m_currentMonitorName = info.name;
  gtk_label_set_text(GTK_LABEL(m_nameLabel), info.name.c_str());
  std::string key =
      "monitors." + info.name + ".custom_name";
  auto &config = bwp::config::ConfigManager::getInstance();
  std::string customName = config.get<std::string>(key, "");
  gtk_editable_set_text(GTK_EDITABLE(m_customNameEntry),
                        customName.c_str());
  std::string res = std::to_string(info.width) + "x" +
                    std::to_string(info.height) + " @ " +
                    std::to_string(info.refresh_rate / 1000) + "Hz";
  gtk_label_set_text(GTK_LABEL(m_resLabel), res.c_str());
  std::string scale = std::to_string((int)(info.scale * 100)) + "%";
  gtk_label_set_text(GTK_LABEL(m_scaleLabel), scale.c_str());
  gtk_widget_set_sensitive(m_box, TRUE);
}
void MonitorConfigPanel::clear() {
  m_currentMonitorName = "";
  gtk_label_set_text(GTK_LABEL(m_nameLabel), "-");
  gtk_editable_set_text(GTK_EDITABLE(m_customNameEntry), "");
  gtk_label_set_text(GTK_LABEL(m_resLabel), "-");
  gtk_label_set_text(GTK_LABEL(m_scaleLabel), "-");
  gtk_widget_set_sensitive(m_box, FALSE);
}
void MonitorConfigPanel::setCallback(ChangeWallpaperCallback callback) {
  m_callback = callback;
}
}  
