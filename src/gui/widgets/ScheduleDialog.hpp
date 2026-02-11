#pragma once
#include "../../core/scheduler/Scheduler.hpp"
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <vector>
namespace bwp::gui {
class ScheduleDialog {
public:
  using SaveCallback =
      std::function<void(const bwp::core::ScheduleEntry &entry)>;
  explicit ScheduleDialog(GtkWindow *parent);
  ~ScheduleDialog();
  void show();
  void show(const bwp::core::ScheduleEntry &entry);
  void setSaveCallback(SaveCallback callback) { m_saveCallback = callback; }
private:
  void setupUi();
  void createTimeSection();
  void createDaysSection();
  void createProfileSection();
  void populateProfileDropdown();
  void populateFromEntry(const bwp::core::ScheduleEntry &entry);
  bwp::core::ScheduleEntry buildEntry() const;
  void onSave();
  GtkWidget *m_dialog = nullptr;
  GtkWidget *m_nameEntry = nullptr;
  GtkWidget *m_startHourSpin = nullptr;
  GtkWidget *m_startMinSpin = nullptr;
  GtkWidget *m_endHourSpin = nullptr;
  GtkWidget *m_endMinSpin = nullptr;
  GtkWidget *m_profileDropdown = nullptr;
  GtkWidget *m_dayToggles[7] = {nullptr};
  std::string m_scheduleId;
  std::vector<std::pair<std::string, std::string>> m_profiles;  
  SaveCallback m_saveCallback;
};
}  
