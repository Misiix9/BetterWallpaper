#pragma once
#include "../widgets/ScheduleDialog.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <string>
namespace bwp::gui {
class ScheduleView {
public:
  ScheduleView();
  ~ScheduleView();
  GtkWidget *getWidget() const { return m_content; }
  void refresh();
private:
  void setupUi();
  void loadSchedules();
  void onCreateSchedule();
  void onEditSchedule(const std::string &scheduleId);
  void onDeleteSchedule(const std::string &scheduleId);
  void onToggleSchedule(const std::string &scheduleId, bool enabled);
  GtkWidget *createScheduleRow(const std::string &id, const std::string &name,
                               const std::string &profileName,
                               const std::string &timeRange, bool enabled);
  GtkWidget *m_content = nullptr;
  GtkWidget *m_scheduleGroup = nullptr;
  GtkWidget *m_emptyState = nullptr;
  std::unique_ptr<ScheduleDialog> m_scheduleDialog;
};
}  
