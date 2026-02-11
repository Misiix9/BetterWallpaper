#pragma once
#include <functional>
#ifndef _WIN32
#include <glib.h>
#endif
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
namespace bwp::core {
struct ScheduleEntry {
  std::string id;
  std::string profileId;
  std::string name;
  int startTime = 0;  
  int endTime = 0;    
  int daysOfWeek = 0b1111111;  
  bool enabled = true;
  bool slideshow = false;
  int slideshowInterval = 300;  
  nlohmann::json toJson() const;
  static ScheduleEntry fromJson(const nlohmann::json &j);
};
class Scheduler {
public:
  using ProfileActivateCallback =
      std::function<void(const std::string &profileId)>;
  static Scheduler &getInstance();
  void start();
  void stop();
  bool isRunning() const { return m_running; }
  void addSchedule(const ScheduleEntry &entry);
  void updateSchedule(const ScheduleEntry &entry);
  void removeSchedule(const std::string &id);
  void setScheduleEnabled(const std::string &id, bool enabled);
  std::vector<ScheduleEntry> getSchedules() const;
  ScheduleEntry *getSchedule(const std::string &id);
  void setProfileActivateCallback(ProfileActivateCallback callback);
  void checkSchedules();
  void loadFromConfig();
  void saveToConfig();
private:
  Scheduler() = default;
  ~Scheduler();
  Scheduler(const Scheduler &) = delete;
  Scheduler &operator=(const Scheduler &) = delete;
  void schedulerLoop();
  bool shouldTrigger(const ScheduleEntry &entry, int currentMinutes,
                     int currentDay) const;
  std::vector<ScheduleEntry> m_schedules;
  ProfileActivateCallback m_activateCallback;
  bool m_running = false;
#ifdef _WIN32
  uintptr_t m_timerId = 0;
#else
  guint m_timerId = 0;
#endif
  std::string m_lastTriggeredProfile;
};
}  
