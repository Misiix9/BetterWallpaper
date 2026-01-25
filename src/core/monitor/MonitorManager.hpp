#pragma once
#include "MonitorInfo.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct wl_display;
struct wl_registry;
struct wl_output;

namespace bwp::monitor {

class WaylandMonitor; // Forward declaration

class MonitorManager {
public:
  static MonitorManager &getInstance();

  bool initialize();
  void update(); // Process events

  std::vector<MonitorInfo> getMonitors() const;
  std::optional<MonitorInfo> getMonitor(const std::string &name) const;
  std::optional<MonitorInfo> getPrimaryMonitor() const;

  using MonitorCallback =
      std::function<void(const MonitorInfo &, bool connected)>;
  void setCallback(MonitorCallback callback);

  // Internal use for Wayland callbacks
  void addMonitor(uint32_t id, wl_output *output);
  void removeMonitor(uint32_t id);

private:
  MonitorManager();
  ~MonitorManager();

  wl_display *m_display = nullptr;
  wl_registry *m_registry = nullptr;

  mutable std::mutex m_mutex;
  std::map<uint32_t, std::shared_ptr<WaylandMonitor>> m_monitors;
  MonitorCallback m_callback;
};

} // namespace bwp::monitor
