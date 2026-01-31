#include "ScheduleView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/scheduler/Scheduler.hpp"
#include "../../core/utils/Logger.hpp"

namespace bwp::gui {

ScheduleView::ScheduleView() {
  setupUi();
  loadSchedules();
}

ScheduleView::~ScheduleView() {}

void ScheduleView::setupUi() {
  m_content = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_content),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_start(box, 24);
  gtk_widget_set_margin_end(box, 24);
  gtk_widget_set_margin_top(box, 24);
  gtk_widget_set_margin_bottom(box, 24);

  // Header
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_bottom(header, 24);

  GtkWidget *title = gtk_label_new("Schedule");
  gtk_widget_add_css_class(title, "title-1");
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_widget_set_hexpand(title, TRUE);
  gtk_box_append(GTK_BOX(header), title);

  GtkWidget *createBtn = gtk_button_new_with_label("New Schedule");
  gtk_button_set_icon_name(GTK_BUTTON(createBtn), "list-add-symbolic");
  gtk_widget_add_css_class(createBtn, "suggested-action");
  g_signal_connect(createBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     ScheduleView *self = static_cast<ScheduleView *>(data);
                     self->onCreateSchedule();
                   }),
                   this);
  gtk_box_append(GTK_BOX(header), createBtn);

  gtk_box_append(GTK_BOX(box), header);

  // Description
  GtkWidget *desc = gtk_label_new(
      "Schedule automatic wallpaper profile changes based on time of day.");
  gtk_widget_add_css_class(desc, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
  gtk_label_set_xalign(GTK_LABEL(desc), 0);
  gtk_widget_set_margin_bottom(desc, 24);
  gtk_box_append(GTK_BOX(box), desc);

  // Empty state
  m_emptyState = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_valign(m_emptyState, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(m_emptyState, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(m_emptyState, 48);
  gtk_widget_set_margin_bottom(m_emptyState, 48);

  GtkWidget *emptyIcon = gtk_image_new_from_icon_name("alarm-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(emptyIcon), 64);
  gtk_widget_add_css_class(emptyIcon, "dim-label");
  gtk_box_append(GTK_BOX(m_emptyState), emptyIcon);

  GtkWidget *emptyTitle = gtk_label_new("No Schedules");
  gtk_widget_add_css_class(emptyTitle, "title-2");
  gtk_box_append(GTK_BOX(m_emptyState), emptyTitle);

  GtkWidget *emptyDesc = gtk_label_new(
      "Create a schedule to automatically change profiles at specific times");
  gtk_widget_add_css_class(emptyDesc, "dim-label");
  gtk_box_append(GTK_BOX(m_emptyState), emptyDesc);

  gtk_box_append(GTK_BOX(box), m_emptyState);

  // Schedules group
  m_scheduleGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(m_scheduleGroup),
                                  "Active Schedules");
  gtk_widget_set_visible(m_scheduleGroup, FALSE);
  gtk_box_append(GTK_BOX(box), m_scheduleGroup);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_content), box);
}

void ScheduleView::loadSchedules() {
  // Clear existing rows
  // For AdwPreferencesGroup we need to be careful about removal

  auto &scheduler = bwp::core::Scheduler::getInstance();
  auto schedules = scheduler.getSchedules();

  if (schedules.empty()) {
    gtk_widget_set_visible(m_emptyState, TRUE);
    gtk_widget_set_visible(m_scheduleGroup, FALSE);
  } else {
    gtk_widget_set_visible(m_emptyState, FALSE);
    gtk_widget_set_visible(m_scheduleGroup, TRUE);

    for (const auto &entry : schedules) {
      // Get profile name
      auto &conf = bwp::config::ConfigManager::getInstance();
      auto profiles = conf.get<std::vector<nlohmann::json>>("profiles.list");

      std::string profileName = "Unknown Profile";
      for (const auto &p : profiles) {
        if (p.value("id", "") == entry.profileId) {
          profileName = p.value("name", "Unknown Profile");
          break;
        }
      }

      // Format time range
      int startHour = entry.startTime / 60;
      int startMin = entry.startTime % 60;
      char timeStr[32];
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", startHour, startMin);
      std::string timeRange = timeStr;

      if (entry.endTime > 0) {
        int endHour = entry.endTime / 60;
        int endMin = entry.endTime % 60;
        snprintf(timeStr, sizeof(timeStr), " - %02d:%02d", endHour, endMin);
        timeRange += timeStr;
      }

      GtkWidget *row = createScheduleRow(entry.id, entry.name, profileName,
                                         timeRange, entry.enabled);
      adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_scheduleGroup), row);
    }
  }
}

void ScheduleView::refresh() { loadSchedules(); }

GtkWidget *ScheduleView::createScheduleRow(const std::string &id,
                                           const std::string &name,
                                           const std::string &profileName,
                                           const std::string &timeRange,
                                           bool enabled) {
  GtkWidget *row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), name.c_str());

  std::string subtitle = profileName + " • " + timeRange;
  adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle.c_str());

  // Enable/disable switch
  GtkWidget *toggle = gtk_switch_new();
  gtk_switch_set_active(GTK_SWITCH(toggle), enabled);
  gtk_widget_set_valign(toggle, GTK_ALIGN_CENTER);

  std::string *idCopy = new std::string(id);
  g_object_set_data_full(
      G_OBJECT(toggle), "schedule-id", idCopy,
      [](gpointer d) { delete static_cast<std::string *>(d); });

  g_signal_connect(toggle, "notify::active",
                   G_CALLBACK(+[](GtkSwitch *sw, GParamSpec *, gpointer data) {
                     ScheduleView *self = static_cast<ScheduleView *>(data);
                     std::string *sid = static_cast<std::string *>(
                         g_object_get_data(G_OBJECT(sw), "schedule-id"));
                     if (sid) {
                       self->onToggleSchedule(*sid, gtk_switch_get_active(sw));
                     }
                   }),
                   this);

  adw_action_row_add_suffix(ADW_ACTION_ROW(row), toggle);

  // Edit button
  GtkWidget *editBtn = gtk_button_new_from_icon_name("document-edit-symbolic");
  gtk_widget_add_css_class(editBtn, "flat");
  gtk_widget_set_valign(editBtn, GTK_ALIGN_CENTER);

  std::string *idCopy2 = new std::string(id);
  g_object_set_data_full(
      G_OBJECT(editBtn), "schedule-id", idCopy2,
      [](gpointer d) { delete static_cast<std::string *>(d); });

  g_signal_connect(editBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                     ScheduleView *self = static_cast<ScheduleView *>(data);
                     std::string *sid = static_cast<std::string *>(
                         g_object_get_data(G_OBJECT(btn), "schedule-id"));
                     if (sid) {
                       self->onEditSchedule(*sid);
                     }
                   }),
                   this);

  adw_action_row_add_suffix(ADW_ACTION_ROW(row), editBtn);

  // Delete button
  GtkWidget *deleteBtn = gtk_button_new_from_icon_name("user-trash-symbolic");
  gtk_widget_add_css_class(deleteBtn, "flat");
  gtk_widget_add_css_class(deleteBtn, "destructive-action");
  gtk_widget_set_valign(deleteBtn, GTK_ALIGN_CENTER);

  std::string *idCopy3 = new std::string(id);
  g_object_set_data_full(
      G_OBJECT(deleteBtn), "schedule-id", idCopy3,
      [](gpointer d) { delete static_cast<std::string *>(d); });

  g_signal_connect(deleteBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                     ScheduleView *self = static_cast<ScheduleView *>(data);
                     std::string *sid = static_cast<std::string *>(
                         g_object_get_data(G_OBJECT(btn), "schedule-id"));
                     if (sid) {
                       self->onDeleteSchedule(*sid);
                     }
                   }),
                   this);

  adw_action_row_add_suffix(ADW_ACTION_ROW(row), deleteBtn);

  return row;
}

void ScheduleView::onCreateSchedule() {
  LOG_INFO("Create new schedule");

  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(m_content));

  if (!m_scheduleDialog) {
    m_scheduleDialog = std::make_unique<ScheduleDialog>(parent);
    m_scheduleDialog->setSaveCallback(
        [this](const bwp::core::ScheduleEntry &entry) {
          bwp::core::Scheduler::getInstance().addSchedule(entry);
          refresh();
          LOG_INFO("Schedule saved: " + entry.name);
        });
  }

  m_scheduleDialog->show();
}

void ScheduleView::onEditSchedule(const std::string &scheduleId) {
  LOG_INFO("Edit schedule: " + scheduleId);

  auto *entry = bwp::core::Scheduler::getInstance().getSchedule(scheduleId);
  if (!entry) {
    LOG_ERROR("Schedule not found: " + scheduleId);
    return;
  }

  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(m_content));

  if (!m_scheduleDialog) {
    m_scheduleDialog = std::make_unique<ScheduleDialog>(parent);
    m_scheduleDialog->setSaveCallback(
        [this](const bwp::core::ScheduleEntry &entry) {
          bwp::core::Scheduler::getInstance().updateSchedule(entry);
          refresh();
          LOG_INFO("Schedule updated: " + entry.name);
        });
  }

  m_scheduleDialog->show(*entry);
}

void ScheduleView::onDeleteSchedule(const std::string &scheduleId) {
  LOG_INFO("Delete schedule: " + scheduleId);
  bwp::core::Scheduler::getInstance().removeSchedule(scheduleId);
  refresh();
}

void ScheduleView::onToggleSchedule(const std::string &scheduleId,
                                    bool enabled) {
  LOG_INFO("Toggle schedule: " + scheduleId + " -> " +
           (enabled ? "ON" : "OFF"));
  bwp::core::Scheduler::getInstance().setScheduleEnabled(scheduleId, enabled);
}

} // namespace bwp::gui
