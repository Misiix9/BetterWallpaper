#include "MonitorConfigPanel.hpp"
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

  // Header
  GtkWidget *header = gtk_label_new("Monitor Settings");
  gtk_widget_add_css_class(header, "title-4");
  gtk_widget_set_halign(header, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_box), header);

  // Info Group
  GtkWidget *group = adw_preferences_group_new();
  gtk_box_append(GTK_BOX(m_box), group);

  // Name Row
  GtkWidget *nameRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(nameRow), "Name");
  m_nameLabel = gtk_label_new("-");
  adw_action_row_add_suffix(ADW_ACTION_ROW(nameRow), m_nameLabel);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), nameRow);

  // Resolution Row
  GtkWidget *resRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(resRow), "Resolution");
  m_resLabel = gtk_label_new("-");
  adw_action_row_add_suffix(ADW_ACTION_ROW(resRow), m_resLabel);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), resRow);

  // Scale Row
  GtkWidget *scaleRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scaleRow), "Scale");
  m_scaleLabel = gtk_label_new("-");
  adw_action_row_add_suffix(ADW_ACTION_ROW(scaleRow), m_scaleLabel);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), scaleRow);

  // Scaling Mode
  GtkWidget *modeRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(modeRow),
                                "Wallpaper Scaling");
  const char *modes[] = {"Fill", "Fit", "Stretch", "Center", "Tile", nullptr};
  GtkStringList *list = gtk_string_list_new(modes);
  adw_combo_row_set_model(ADW_COMBO_ROW(modeRow), G_LIST_MODEL(list));
  g_object_unref(list);
  m_modeDropdown = modeRow;
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), modeRow);

  // Actions
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

void MonitorConfigPanel::setMonitor(const bwp::monitor::MonitorInfo &info) {
  m_currentMonitorName = info.name;

  gtk_label_set_text(GTK_LABEL(m_nameLabel), info.name.c_str());

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
  gtk_label_set_text(GTK_LABEL(m_resLabel), "-");
  gtk_label_set_text(GTK_LABEL(m_scaleLabel), "-");
  gtk_widget_set_sensitive(m_box, FALSE);
}

void MonitorConfigPanel::setCallback(ChangeWallpaperCallback callback) {
  m_callback = callback;
}

} // namespace bwp::gui
