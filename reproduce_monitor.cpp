#include "src/core/monitor/MonitorManager.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  std::cout << "Initializing MonitorManager..." << std::endl;
  if (!bwp::monitor::MonitorManager::getInstance().initialize()) {
    std::cerr << "Failed to initialize MonitorManager" << std::endl;
    return 1;
  }

  // Wait for events (roundtrips are done in initialize, but let's give it a
  // moment if async)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();
  std::cout << "Detected " << monitors.size() << " monitors." << std::endl;

  for (const auto &monitor : monitors) {
    std::cout << "Monitor: " << monitor.name << " (" << monitor.description
              << ")" << std::endl;
  }

  if (monitors.empty()) {
    std::cout << "No monitors detected!" << std::endl;
    return 1;
  }

  return 0;
}
