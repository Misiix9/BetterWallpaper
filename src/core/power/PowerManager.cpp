#include "PowerManager.hpp"
#include "../utils/Logger.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
namespace bwp::core {
PowerManager::~PowerManager() { stopMonitoring(); }
void PowerManager::startMonitoring() {
  if (m_timerId > 0)
    return;
  m_lastState = readBatteryState();
  m_timerId = g_timeout_add_seconds(
      10,
      +[](gpointer data) -> gboolean {
        PowerManager *self = static_cast<PowerManager *>(data);
        self->checkBattery();
        return TRUE;
      },
      this);
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "PowerManager: Monitoring started");
}
void PowerManager::stopMonitoring() {
  if (m_timerId > 0) {
    g_source_remove(m_timerId);
    m_timerId = 0;
  }
}
bool PowerManager::isOnBattery() const { return readBatteryState(); }
void PowerManager::addCallback(Callback cb) {
  std::lock_guard<std::mutex> lock(m_cbMutex);
  m_callbacks.push_back(cb);
}
void PowerManager::checkBattery() {
  bool newState = readBatteryState();
  if (newState != m_lastState) {
    m_lastState = newState;
    bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                            std::string("Power state changed: ") +
                                (newState ? "Battery" : "AC"));
    std::lock_guard<std::mutex> lock(m_cbMutex);
    for (auto &cb : m_callbacks) {
      cb(newState);
    }
  }
}
bool PowerManager::readBatteryState() const {
  try {
    if (!std::filesystem::exists("/sys/class/power_supply")) {
      return false;
    }
    for (const auto &entry :
         std::filesystem::directory_iterator("/sys/class/power_supply")) {
      std::string name = entry.path().filename().string();
      if (name.find("BAT") != std::string::npos) {
        std::ifstream statusFile(entry.path() / "status");
        if (statusFile.is_open()) {
          std::string status;
          std::getline(statusFile, status);
          if (status == "Discharging") {
            return true;
          }
        }
      }
    }
  } catch (const std::exception &e) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARN,
                            std::string("readBatteryState error: ") + e.what());
  }
  return false;
}
} // namespace bwp::core
