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

  // Initial check
  m_lastState = readBatteryState();

  // Check every 10 seconds
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

bool PowerManager::isOnBattery() const {
  // This might be slightly stale if called between updates, but safe
  // Ideally we could force read, but cached is fine for UI
  return const_cast<PowerManager *>(this)->readBatteryState();
}

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

bool PowerManager::readBatteryState() {
  // Scan /sys/class/power_supply
  if (!std::filesystem::exists("/sys/class/power_supply")) {
    return false; // Desktop assumed
  }

  for (const auto &entry :
       std::filesystem::directory_iterator("/sys/class/power_supply")) {
    std::string name = entry.path().filename().string();
    // Look for BAT* or similar? Or just check type?
    // Usually BAT0, BAT1.
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
  return false;
}

} // namespace bwp::core
