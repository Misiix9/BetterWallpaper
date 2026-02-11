#include "Scheduler.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#ifndef _WIN32
#include <gtk/gtk.h>
#endif
namespace bwp::core {
nlohmann::json ScheduleEntry::toJson() const {
  return {{"id", id},
          {"profileId", profileId},
          {"name", name},
          {"startTime", startTime},
          {"endTime", endTime},
          {"daysOfWeek", daysOfWeek},
          {"enabled", enabled},
          {"slideshow", slideshow},
          {"slideshowInterval", slideshowInterval}};
}
ScheduleEntry ScheduleEntry::fromJson(const nlohmann::json &j) {
  ScheduleEntry entry;
  entry.id = j.value("id", "");
  entry.profileId = j.value("profileId", "");
  entry.name = j.value("name", "");
  entry.startTime = j.value("startTime", 0);
  entry.endTime = j.value("endTime", 0);
  entry.daysOfWeek = j.value("daysOfWeek", 0b1111111);
  entry.enabled = j.value("enabled", true);
  entry.slideshow = j.value("slideshow", false);
  entry.slideshowInterval = j.value("slideshowInterval", 300);
  return entry;
}
Scheduler &Scheduler::getInstance() {
  static Scheduler instance;
  return instance;
}
Scheduler::~Scheduler() { stop(); }
void Scheduler::start() {
  if (m_running)
    return;
  loadFromConfig();
  m_running = true;
#ifdef _WIN32
#else
  m_timerId = g_timeout_add_seconds(
      60,
      [](gpointer data) -> gboolean {
        auto *self = static_cast<Scheduler *>(data);
        self->checkSchedules();
        return G_SOURCE_CONTINUE;
      },
      this);
#endif
  LOG_INFO("Scheduler started with " + std::to_string(m_schedules.size()) +
           " schedules");
  checkSchedules();
}
void Scheduler::stop() {
  if (!m_running)
    return;
  if (m_timerId > 0) {
#ifndef _WIN32
    g_source_remove(m_timerId);
#endif
    m_timerId = 0;
  }
  m_running = false;
  LOG_INFO("Scheduler stopped");
}
void Scheduler::addSchedule(const ScheduleEntry &entry) {
  m_schedules.push_back(entry);
  saveToConfig();
}
void Scheduler::updateSchedule(const ScheduleEntry &entry) {
  for (auto &s : m_schedules) {
    if (s.id == entry.id) {
      s = entry;
      saveToConfig();
      return;
    }
  }
}
void Scheduler::removeSchedule(const std::string &id) {
  m_schedules.erase(
      std::remove_if(m_schedules.begin(), m_schedules.end(),
                     [&id](const ScheduleEntry &e) { return e.id == id; }),
      m_schedules.end());
  saveToConfig();
}
void Scheduler::setScheduleEnabled(const std::string &id, bool enabled) {
  for (auto &s : m_schedules) {
    if (s.id == id) {
      s.enabled = enabled;
      saveToConfig();
      return;
    }
  }
}
std::vector<ScheduleEntry> Scheduler::getSchedules() const {
  return m_schedules;
}
ScheduleEntry *Scheduler::getSchedule(const std::string &id) {
  for (auto &s : m_schedules) {
    if (s.id == id) {
      return &s;
    }
  }
  return nullptr;
}
void Scheduler::setProfileActivateCallback(ProfileActivateCallback callback) {
  m_activateCallback = callback;
}
void Scheduler::checkSchedules() {
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm localTimeBuf{};
  std::tm *localTime = localtime_r(&time, &localTimeBuf);
  if (!localTime) {
    LOG_ERROR("Scheduler: localtime_r failed");
    return;
  }
  int currentMinutes = localTime->tm_hour * 60 + localTime->tm_min;
  int currentDay = localTime->tm_wday;  
  for (const auto &entry : m_schedules) {
    if (!entry.enabled)
      continue;
    if (shouldTrigger(entry, currentMinutes, currentDay)) {
      if (m_lastTriggeredProfile != entry.profileId) {
        LOG_INFO("Schedule trigger: " + entry.name + " -> " + entry.profileId);
        m_lastTriggeredProfile = entry.profileId;
        if (m_activateCallback) {
          m_activateCallback(entry.profileId);
        }
      }
    }
  }
}
bool Scheduler::shouldTrigger(const ScheduleEntry &entry, int currentMinutes,
                              int currentDay) const {
  int dayBit = 1 << currentDay;
  if ((entry.daysOfWeek & dayBit) == 0) {
    return false;
  }
  if (entry.endTime == 0) {
    return currentMinutes >= entry.startTime;
  }
  if (entry.endTime < entry.startTime) {
    return currentMinutes >= entry.startTime || currentMinutes < entry.endTime;
  }
  return currentMinutes >= entry.startTime && currentMinutes < entry.endTime;
}
void Scheduler::loadFromConfig() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto scheduleJson = conf.get<std::vector<nlohmann::json>>("schedules.list");
  m_schedules.clear();
  for (const auto &j : scheduleJson) {
    m_schedules.push_back(ScheduleEntry::fromJson(j));
  }
  LOG_DEBUG("Loaded " + std::to_string(m_schedules.size()) + " schedules");
}
void Scheduler::saveToConfig() {
  std::vector<nlohmann::json> scheduleJson;
  for (const auto &entry : m_schedules) {
    scheduleJson.push_back(entry.toJson());
  }
  auto &conf = bwp::config::ConfigManager::getInstance();
  conf.set("schedules.list", scheduleJson);
}
}  
