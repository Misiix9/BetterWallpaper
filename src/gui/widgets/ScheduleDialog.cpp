#include "ScheduleDialog.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/utils/Logger.hpp"
#include <chrono>
namespace bwp::gui {
ScheduleDialog::ScheduleDialog(GtkWindow *parent) {
  (void)parent;
  m_dialog = GTK_WIDGET(adw_dialog_new());
  adw_dialog_set_title(ADW_DIALOG(m_dialog), "New Schedule");
  adw_dialog_set_content_width(ADW_DIALOG(m_dialog), 450);
  adw_dialog_set_content_height(ADW_DIALOG(m_dialog), 500);
  setupUi();
}
ScheduleDialog::~ScheduleDialog() {}
void ScheduleDialog::setupUi() {
  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *headerBar = adw_header_bar_new();
  adw_header_bar_set_show_start_title_buttons(ADW_HEADER_BAR(headerBar), FALSE);
  adw_header_bar_set_show_end_title_buttons(ADW_HEADER_BAR(headerBar), FALSE);
  GtkWidget *cancelBtn = gtk_button_new_with_label("Cancel");
  g_signal_connect(cancelBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<ScheduleDialog *>(data);
                     adw_dialog_close(ADW_DIALOG(self->m_dialog));
                   }),
                   this);
  adw_header_bar_pack_start(ADW_HEADER_BAR(headerBar), cancelBtn);
  GtkWidget *saveBtn = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(saveBtn, "suggested-action");
  g_signal_connect(saveBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<ScheduleDialog *>(data);
                     self->onSave();
                   }),
                   this);
  adw_header_bar_pack_end(ADW_HEADER_BAR(headerBar), saveBtn);
  gtk_box_append(GTK_BOX(content), headerBar);
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scrolled, TRUE);
  GtkWidget *innerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_margin_start(innerBox, 16);
  gtk_widget_set_margin_end(innerBox, 16);
  gtk_widget_set_margin_top(innerBox, 16);
  gtk_widget_set_margin_bottom(innerBox, 16);
  GtkWidget *nameGroup = adw_preferences_group_new();
  m_nameEntry = adw_entry_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_nameEntry),
                                "Schedule Name");
  gtk_editable_set_text(GTK_EDITABLE(m_nameEntry), "New Schedule");
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(nameGroup), m_nameEntry);
  gtk_box_append(GTK_BOX(innerBox), nameGroup);
  createProfileSection();
  gtk_box_append(GTK_BOX(innerBox), m_profileDropdown);
  createTimeSection();
  createDaysSection();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), innerBox);
  gtk_box_append(GTK_BOX(content), scrolled);
  adw_dialog_set_child(ADW_DIALOG(m_dialog), content);
}
void ScheduleDialog::createTimeSection() {
  GtkWidget *timeGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(timeGroup), "Time");
  GtkWidget *startRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(startRow), "Start Time");
  GtkWidget *startBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_valign(startBox, GTK_ALIGN_CENTER);
  m_startHourSpin = gtk_spin_button_new_with_range(0, 23, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_startHourSpin), 9);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(m_startHourSpin), TRUE);
  gtk_widget_set_size_request(m_startHourSpin, 60, -1);
  GtkWidget *colonLabel1 = gtk_label_new(":");
  m_startMinSpin = gtk_spin_button_new_with_range(0, 59, 5);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_startMinSpin), 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(m_startMinSpin), TRUE);
  gtk_widget_set_size_request(m_startMinSpin, 60, -1);
  gtk_box_append(GTK_BOX(startBox), m_startHourSpin);
  gtk_box_append(GTK_BOX(startBox), colonLabel1);
  gtk_box_append(GTK_BOX(startBox), m_startMinSpin);
  adw_action_row_add_suffix(ADW_ACTION_ROW(startRow), startBox);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(timeGroup), startRow);
  GtkWidget *endRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(endRow), "End Time");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(endRow),
                              "Leave at 00:00 for no end");
  GtkWidget *endBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_valign(endBox, GTK_ALIGN_CENTER);
  m_endHourSpin = gtk_spin_button_new_with_range(0, 23, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_endHourSpin), 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(m_endHourSpin), TRUE);
  gtk_widget_set_size_request(m_endHourSpin, 60, -1);
  GtkWidget *colonLabel2 = gtk_label_new(":");
  m_endMinSpin = gtk_spin_button_new_with_range(0, 59, 5);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_endMinSpin), 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(m_endMinSpin), TRUE);
  gtk_widget_set_size_request(m_endMinSpin, 60, -1);
  gtk_box_append(GTK_BOX(endBox), m_endHourSpin);
  gtk_box_append(GTK_BOX(endBox), colonLabel2);
  gtk_box_append(GTK_BOX(endBox), m_endMinSpin);
  adw_action_row_add_suffix(ADW_ACTION_ROW(endRow), endBox);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(timeGroup), endRow);
  GtkWidget *parent = gtk_widget_get_parent(m_nameEntry);
  if (parent) {
    parent = gtk_widget_get_parent(parent);  
    if (parent) {
      gtk_box_append(GTK_BOX(parent), timeGroup);
    }
  }
}
void ScheduleDialog::createDaysSection() {
  GtkWidget *daysGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(daysGroup), "Days");
  GtkWidget *daysRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(daysRow), "Active Days");
  GtkWidget *daysBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_valign(daysBox, GTK_ALIGN_CENTER);
  const char *dayLabels[] = {"S", "M", "T", "W", "T", "F", "S"};
  for (int i = 0; i < 7; ++i) {
    m_dayToggles[i] = gtk_toggle_button_new_with_label(dayLabels[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_dayToggles[i]), TRUE);
    gtk_widget_add_css_class(m_dayToggles[i], "circular");
    gtk_widget_set_size_request(m_dayToggles[i], 36, 36);
    gtk_box_append(GTK_BOX(daysBox), m_dayToggles[i]);
  }
  adw_action_row_add_suffix(ADW_ACTION_ROW(daysRow), daysBox);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(daysGroup), daysRow);
  GtkWidget *parent = gtk_widget_get_parent(m_nameEntry);
  if (parent) {
    parent = gtk_widget_get_parent(parent);
    if (parent) {
      gtk_box_append(GTK_BOX(parent), daysGroup);
    }
  }
}
void ScheduleDialog::createProfileSection() {
  populateProfileDropdown();
  GtkStringList *profileList = gtk_string_list_new(nullptr);
  for (const auto &p : m_profiles) {
    gtk_string_list_append(profileList, p.second.c_str());
  }
  m_profileDropdown = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_profileDropdown),
                                "Profile");
  adw_combo_row_set_model(ADW_COMBO_ROW(m_profileDropdown),
                          G_LIST_MODEL(profileList));
}
void ScheduleDialog::populateProfileDropdown() {
  m_profiles.clear();
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto profilesJson = conf.get<std::vector<nlohmann::json>>("profiles.list");
  for (const auto &p : profilesJson) {
    std::string id = p.value("id", "");
    std::string name = p.value("name", "Unnamed");
    if (!id.empty()) {
      m_profiles.push_back({id, name});
    }
  }
  if (m_profiles.empty()) {
    m_profiles.push_back({"", "(No profiles - create one first)"});
  }
}
void ScheduleDialog::populateFromEntry(const bwp::core::ScheduleEntry &entry) {
  m_scheduleId = entry.id;
  gtk_editable_set_text(GTK_EDITABLE(m_nameEntry), entry.name.c_str());
  int startHour = entry.startTime / 60;
  int startMin = entry.startTime % 60;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_startHourSpin), startHour);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_startMinSpin), startMin);
  int endHour = entry.endTime / 60;
  int endMin = entry.endTime % 60;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_endHourSpin), endHour);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_endMinSpin), endMin);
  for (int i = 0; i < 7; ++i) {
    bool active = (entry.daysOfWeek & (1 << i)) != 0;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_dayToggles[i]), active);
  }
  for (size_t i = 0; i < m_profiles.size(); ++i) {
    if (m_profiles[i].first == entry.profileId) {
      adw_combo_row_set_selected(ADW_COMBO_ROW(m_profileDropdown), i);
      break;
    }
  }
}
bwp::core::ScheduleEntry ScheduleDialog::buildEntry() const {
  bwp::core::ScheduleEntry entry;
  if (m_scheduleId.empty()) {
    entry.id = "schedule_" +
               std::to_string(
                   std::chrono::system_clock::now().time_since_epoch().count());
  } else {
    entry.id = m_scheduleId;
  }
  entry.name = gtk_editable_get_text(GTK_EDITABLE(m_nameEntry));
  int startHour = static_cast<int>(
      gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_startHourSpin)));
  int startMin = static_cast<int>(
      gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_startMinSpin)));
  entry.startTime = startHour * 60 + startMin;
  int endHour = static_cast<int>(
      gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_endHourSpin)));
  int endMin = static_cast<int>(
      gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_endMinSpin)));
  entry.endTime = endHour * 60 + endMin;
  entry.daysOfWeek = 0;
  for (int i = 0; i < 7; ++i) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_dayToggles[i]))) {
      entry.daysOfWeek |= (1 << i);
    }
  }
  guint selected = adw_combo_row_get_selected(ADW_COMBO_ROW(m_profileDropdown));
  if (selected < m_profiles.size()) {
    entry.profileId = m_profiles[selected].first;
  }
  entry.enabled = true;
  return entry;
}
void ScheduleDialog::onSave() {
  bwp::core::ScheduleEntry entry = buildEntry();
  if (m_saveCallback) {
    m_saveCallback(entry);
  }
  adw_dialog_close(ADW_DIALOG(m_dialog));
}
void ScheduleDialog::show() {
  bwp::core::ScheduleEntry emptyEntry;
  emptyEntry.name = "New Schedule";
  emptyEntry.startTime = 9 * 60;      
  emptyEntry.daysOfWeek = 0b1111111;  
  show(emptyEntry);
}
void ScheduleDialog::show(const bwp::core::ScheduleEntry &entry) {
  populateFromEntry(entry);
  if (entry.id.empty()) {
    adw_dialog_set_title(ADW_DIALOG(m_dialog), "New Schedule");
  } else {
    adw_dialog_set_title(ADW_DIALOG(m_dialog), "Edit Schedule");
  }
  adw_dialog_present(ADW_DIALOG(m_dialog), nullptr);
}
}  
