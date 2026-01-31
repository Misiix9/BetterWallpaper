#pragma once
#include <atomic>
#include <functional>
#include <gtk/gtk.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace bwp::core {

class PowerManager {
public:
  static PowerManager &getInstance() {
    static PowerManager instance;
    return instance;
  }

  void startMonitoring();
  void stopMonitoring();

  // Check if currently on battery and discharging
  bool isOnBattery() const;

  using Callback = std::function<void(bool onBattery)>;
  void addCallback(Callback cb);

private:
  PowerManager() = default;
  ~PowerManager();

  void checkBattery();
  bool readBatteryState(); // Returns true if discharging

  std::vector<Callback> m_callbacks;
  std::mutex m_cbMutex;

  guint m_timerId = 0;
  bool m_lastState = false; // false = AC, true = Battery
};

} // namespace bwp::core
