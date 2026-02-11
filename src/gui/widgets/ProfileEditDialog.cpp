#include "ProfileEditDialog.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/utils/Logger.hpp"
#include <chrono>
namespace bwp::gui {
ProfileEditDialog::ProfileEditDialog(GtkWindow *parent) {
  (void)parent;  
  m_dialog = GTK_WIDGET(adw_dialog_new());
  adw_dialog_set_title(ADW_DIALOG(m_dialog), "Edit Profile");
  adw_dialog_set_content_width(ADW_DIALOG(m_dialog), 500);
  adw_dialog_set_content_height(ADW_DIALOG(m_dialog), 450);
  setupUi();
}
ProfileEditDialog::~ProfileEditDialog() {}
void ProfileEditDialog::setupUi() {
  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *headerBar = adw_header_bar_new();
  adw_header_bar_set_show_start_title_buttons(ADW_HEADER_BAR(headerBar), FALSE);
  adw_header_bar_set_show_end_title_buttons(ADW_HEADER_BAR(headerBar), FALSE);
  GtkWidget *cancelBtn = gtk_button_new_with_label("Cancel");
  g_signal_connect(cancelBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<ProfileEditDialog *>(data);
                     adw_dialog_close(ADW_DIALOG(self->m_dialog));
                   }),
                   this);
  adw_header_bar_pack_start(ADW_HEADER_BAR(headerBar), cancelBtn);
  GtkWidget *saveBtn = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(saveBtn, "suggested-action");
  g_signal_connect(saveBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<ProfileEditDialog *>(data);
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
  createBasicInfoSection();
  gtk_box_append(GTK_BOX(innerBox), m_nameEntry);
  gtk_box_append(GTK_BOX(innerBox), m_descEntry);
  createMonitorSection();
  gtk_box_append(GTK_BOX(innerBox), m_monitorGroup);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), innerBox);
  gtk_box_append(GTK_BOX(content), scrolled);
  adw_dialog_set_child(ADW_DIALOG(m_dialog), content);
}
void ProfileEditDialog::createBasicInfoSection() {
  m_nameEntry = adw_entry_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_nameEntry),
                                "Profile Name");
  gtk_editable_set_text(GTK_EDITABLE(m_nameEntry), "New Profile");
  m_descEntry = adw_entry_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_descEntry),
                                "Description");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(m_descEntry), "Optional");
}
void ProfileEditDialog::createMonitorSection() {
  m_monitorGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(m_monitorGroup),
                                  "Monitor Wallpapers");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(m_monitorGroup),
      "Configure wallpapers for each monitor in this profile");
  auto &mm = bwp::monitor::MonitorManager::getInstance();
  auto monitors = mm.getMonitors();
  if (monitors.empty()) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                  "No monitors detected");
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(row),
        "Monitor configuration will be available when monitors are detected");
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_monitorGroup), row);
  } else {
    for (const auto &mon : monitors) {
      GtkWidget *row = adw_action_row_new();
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), mon.name.c_str());
      std::string resText =
          std::to_string(mon.width) + "x" + std::to_string(mon.height);
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), resText.c_str());
      GtkWidget *chooseBtn = gtk_button_new_with_label("Choose");
      gtk_widget_set_valign(chooseBtn, GTK_ALIGN_CENTER);
      std::string *monName = new std::string(mon.name);
      g_object_set_data_full(
          G_OBJECT(chooseBtn), "monitor-name", monName,
          [](gpointer d) { delete static_cast<std::string *>(d); });
      g_object_set_data(G_OBJECT(row), "monitor-name",
                        g_object_get_data(G_OBJECT(chooseBtn), "monitor-name"));
      g_signal_connect(chooseBtn, "clicked",
                       G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                         auto *self = static_cast<ProfileEditDialog *>(data);
                         (void)self;
                         std::string *name = static_cast<std::string *>(
                             g_object_get_data(G_OBJECT(btn), "monitor-name"));
                         if (name) {
                           LOG_INFO("Choose wallpaper for monitor: " + *name);
                         }
                       }),
                       this);
      adw_action_row_add_suffix(ADW_ACTION_ROW(row), chooseBtn);
      GtkWidget *clearBtn =
          gtk_button_new_from_icon_name("edit-clear-symbolic");
      gtk_widget_add_css_class(clearBtn, "flat");
      gtk_widget_set_valign(clearBtn, GTK_ALIGN_CENTER);
      adw_action_row_add_suffix(ADW_ACTION_ROW(row), clearBtn);
      adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_monitorGroup), row);
    }
  }
}
void ProfileEditDialog::populateFromProfile(const nlohmann::json &profile) {
  m_currentProfile = profile;
  m_profileId = profile.value("id", "");
  std::string name = profile.value("name", "New Profile");
  std::string desc = profile.value("description", "");
  gtk_editable_set_text(GTK_EDITABLE(m_nameEntry), name.c_str());
  gtk_editable_set_text(GTK_EDITABLE(m_descEntry), desc.c_str());
}
nlohmann::json ProfileEditDialog::buildProfile() const {
  nlohmann::json profile = m_currentProfile;
  if (m_profileId.empty()) {
    profile["id"] =
        "profile_" +
        std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
  } else {
    profile["id"] = m_profileId;
  }
  profile["name"] = gtk_editable_get_text(GTK_EDITABLE(m_nameEntry));
  profile["description"] = gtk_editable_get_text(GTK_EDITABLE(m_descEntry));
  return profile;
}
void ProfileEditDialog::onSave() {
  nlohmann::json profile = buildProfile();
  if (m_saveCallback) {
    m_saveCallback(profile);
  }
  adw_dialog_close(ADW_DIALOG(m_dialog));
}
void ProfileEditDialog::show() {
  nlohmann::json emptyProfile;
  emptyProfile["name"] = "New Profile";
  emptyProfile["description"] = "";
  emptyProfile["monitors"] = nlohmann::json::object();
  show(emptyProfile);
}
void ProfileEditDialog::show(const nlohmann::json &profile) {
  populateFromProfile(profile);
  if (profile.contains("id") && !profile["id"].get<std::string>().empty()) {
    adw_dialog_set_title(ADW_DIALOG(m_dialog), "Edit Profile");
  } else {
    adw_dialog_set_title(ADW_DIALOG(m_dialog), "New Profile");
  }
  adw_dialog_present(ADW_DIALOG(m_dialog), nullptr);
}
}  
