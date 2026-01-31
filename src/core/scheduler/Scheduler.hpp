#pragma once
#include <functional>
#include <glib.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace bwp::core {

/**
 * Schedule entry representing a time-based profile trigger.
 */
struct ScheduleEntry {
  std::string id;
  std::string profileId;
  std::string name;

  // Time of day (minutes from midnight)
  int startTime = 0; // e.g., 540 = 9:00 AM
  int endTime = 0;   // e.g., 1020 = 5:00 PM (0 = no end)

  // Days of week (bit flags: Sun=1, Mon=2, Tue=4, ...)
  int daysOfWeek = 0b1111111; // All days by default

  bool enabled = true;

  // For slideshow mode
  bool slideshow = false;
  int slideshowInterval = 300; // seconds

  nlohmann::json toJson() const;
  static ScheduleEntry fromJson(const nlohmann::json &j);
};

/**
 * Scheduler manages time-based profile and wallpaper changes.
 * Runs in the background and triggers profile activations based on schedules.
 */
class Scheduler {
public:
  using ProfileActivateCallback =
      std::function<void(const std::string &profileId)>;

  static Scheduler &getInstance();

  // Initialize and start the scheduler
  void start();
  void stop();
  bool isRunning() const { return m_running; }

  // Schedule management
  void addSchedule(const ScheduleEntry &entry);
  void updateSchedule(const ScheduleEntry &entry);
  void removeSchedule(const std::string &id);
  void setScheduleEnabled(const std::string &id, bool enabled);

  std::vector<ScheduleEntry> getSchedules() const;
  ScheduleEntry *getSchedule(const std::string &id);

  // Callback when a profile should be activated
  void setProfileActivateCallback(ProfileActivateCallback callback);

  // Manual check (for testing)
  void checkSchedules();

  // Load/save from config
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
  guint m_timerId = 0;

  std::string m_lastTriggeredProfile;
};

} // namespace bwp::core
