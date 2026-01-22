#include "MonitorManager.hpp"
#include "../utils/Logger.hpp"
#include "WaylandMonitor.hpp"
#include <cstring>
#include <iostream>
#include <wayland-client.h>

namespace bwp::monitor {

// Registry listeners
static void registry_handle_global(void *data, wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version) {
  auto *manager = static_cast<MonitorManager *>(data);
  if (strcmp(interface, wl_output_interface.name) == 0) {
    // Bind to version 4 if possible for name/desc, otherwise 1
    uint32_t version_to_bind = (version >= 4) ? 4 : version;
    auto *output = static_cast<wl_output *>(wl_registry_bind(
        registry, name, &wl_output_interface, version_to_bind));
    manager->addMonitor(name, output);
  }
}

static void registry_handle_global_remove(void *data, wl_registry *,
                                          uint32_t name) {
  auto *manager = static_cast<MonitorManager *>(data);
  manager->removeMonitor(name);
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global, registry_handle_global_remove};

MonitorManager &MonitorManager::getInstance() {
  static MonitorManager instance;
  return instance;
}

MonitorManager::MonitorManager() = default;

MonitorManager::~MonitorManager() {
  if (m_registry)
    wl_registry_destroy(m_registry);
  if (m_display)
    wl_display_disconnect(m_display);
}

bool MonitorManager::initialize() {
  m_display = wl_display_connect(nullptr);
  if (!m_display) {
    LOG_ERROR("Failed to connect to Wayland display");
    return false;
  }

  m_registry = wl_display_get_registry(m_display);
  wl_registry_add_listener(m_registry, &registry_listener, this);

  // Roundtrip to get initials
  wl_display_roundtrip(m_display);

  // Roundtrip again to get events
  wl_display_roundtrip(m_display);

  return true;
}

void MonitorManager::update() {
  if (m_display) {
    wl_display_dispatch(m_display);
  }
}

void MonitorManager::addMonitor(uint32_t id, wl_output *output) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto monitor = std::make_shared<WaylandMonitor>(id, output);
  m_monitors[id] = monitor;
  LOG_INFO("Monitor detected with ID " + std::to_string(id));

  if (m_callback) {
    m_callback(monitor->getInfo(), true);
  }
}

void MonitorManager::removeMonitor(uint32_t id) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_monitors.count(id)) {
    MonitorInfo info = m_monitors[id]->getInfo();
    m_monitors.erase(id);
    LOG_INFO("Monitor removed with ID " + std::to_string(id));

    if (m_callback) {
      m_callback(info, false);
    }
  }
}

std::vector<MonitorInfo> MonitorManager::getMonitors() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<MonitorInfo> result;
  for (const auto &[id, monitor] : m_monitors) {
    result.push_back(monitor->getInfo());
  }
  return result;
}

std::optional<MonitorInfo>
MonitorManager::getMonitor(const std::string &name) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto &[id, monitor] : m_monitors) {
    if (monitor->getInfo().name == name) {
      return monitor->getInfo();
    }
  }
  return std::nullopt;
}

std::optional<MonitorInfo> MonitorManager::getPrimaryMonitor() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  // Wayland doesn't explicitly have "primary" concept in standard protocol
  // consistently exposed easily, usually position 0,0 is primary logic or
  // compositor specific. For now, return first.
  if (!m_monitors.empty()) {
    return m_monitors.begin()->second->getInfo();
  }
  return std::nullopt;
}

void MonitorManager::setCallback(MonitorCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_callback = callback;
}

} // namespace bwp::monitor
