#include "MonitorConfigPanel.hpp"
#include <format>

namespace bwp::gui {

MonitorConfigPanel::MonitorConfigPanel() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(m_box, 24);
  gtk_widget_set_margin_end(m_box, 24);
  gtk_widget_set_margin_top(m_box, 12);
  gtk_widget_set_margin_bottom(m_box, 12);

  // Header Info
  m_titleLabel = gtk_label_new("No Monitor Selected");
  gtk_widget_add_css_class(m_titleLabel, "title-2");
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_box), m_titleLabel);

  m_specsLabel = gtk_label_new("");
  gtk_widget_add_css_class(m_specsLabel, "dim-label");
  gtk_widget_set_halign(m_specsLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_bottom(m_specsLabel, 12);
  gtk_box_append(GTK_BOX(m_box), m_specsLabel);

  // Group
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Settings");
  gtk_box_append(GTK_BOX(m_box), group);

  // Scaling Mode
  m_scalingRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_scalingRow),
                                "Scaling Mode");

  const char *modes[] = {"Stretch", "Fill", "Fit",  "Center",
                         "Tile",    "Zoom", nullptr};
  GtkStringList *model = gtk_string_list_new(modes);
  adw_combo_row_set_model(ADW_COMBO_ROW(m_scalingRow), G_LIST_MODEL(model));

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), m_scalingRow);

  // Separation for Wallpaper Engine / Video specific settings
  // Volume
  m_volumeRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_volumeRow), "Volume");

  GtkWidget *volScale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_widget_set_hexpand(volScale, TRUE);
  gtk_widget_set_size_request(volScale, 150, -1);
  adw_action_row_add_suffix(ADW_ACTION_ROW(m_volumeRow), volScale);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), m_volumeRow);

  // Speed
  m_speedRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_speedRow),
                                "Playback Speed");

  GtkWidget *speedScale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.25, 2.0, 0.25);
  gtk_widget_set_hexpand(speedScale, TRUE);
  gtk_widget_set_size_request(speedScale, 150, -1);
  gtk_range_set_value(GTK_RANGE(speedScale), 1.0);
  adw_action_row_add_suffix(ADW_ACTION_ROW(m_speedRow), speedScale);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), m_speedRow);
}

MonitorConfigPanel::~MonitorConfigPanel() {}

void MonitorConfigPanel::setMonitor(const bwp::monitor::MonitorInfo &info) {
  m_currentMonitor = info;
  gtk_label_set_text(GTK_LABEL(m_titleLabel), info.name.c_str());

  std::string specs = std::to_string(info.width) + "x" +
                      std::to_string(info.height) + " @ " +
                      std::to_string(info.refresh_rate / 1000) + "Hz";
  gtk_label_set_text(GTK_LABEL(m_specsLabel), specs.c_str());
}

void MonitorConfigPanel::setWallpaperInfo(
    const bwp::wallpaper::WallpaperInfo &info) {
  // Enable/Disable rows based on type
  bool isVideo =
      (info.type == bwp::wallpaper::WallpaperType::Video ||
       info.type == bwp::wallpaper::WallpaperType::WEVideo ||
       info.type ==
           bwp::wallpaper::WallpaperType::WEScene); // Scene might have audio

  gtk_widget_set_sensitive(m_volumeRow, isVideo);
  gtk_widget_set_sensitive(m_speedRow, isVideo);
}

} // namespace bwp::gui
