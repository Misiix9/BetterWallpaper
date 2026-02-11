#include "MonitorManager.hpp"
#include "../utils/Logger.hpp"
#ifndef _WIN32
#include "WaylandMonitor.hpp"
#endif
#include <cstring>
#include <iostream>
#ifndef _WIN32
#include <wayland-client.h>
#endif
namespace bwp::monitor {
#ifndef _WIN32
static void registry_handle_global(void *data, wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version) {
  auto *manager = static_cast<MonitorManager *>(data);
  if (strcmp(interface, wl_output_interface.name) == 0) {
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
#endif
MonitorManager &MonitorManager::getInstance() {
  static MonitorManager instance;
  return instance;
}
MonitorManager::MonitorManager() = default;
MonitorManager::~MonitorManager() {
#ifndef _WIN32
  if (m_registry)
    wl_registry_destroy(m_registry);
  m_monitors.clear();  
  if (m_display)
    wl_display_disconnect(m_display);
#endif
}
bool MonitorManager::initialize() {
#ifdef _WIN32
  LOG_INFO("MonitorManager initialized (Windows Stub).");
  return true;
#else
  if (m_display) {
    return true;
  }
  m_display = wl_display_connect(nullptr);
  if (!m_display) {
    LOG_ERROR("Failed to connect to Wayland display");
    return false;
  }
  m_registry = wl_display_get_registry(m_display);
  wl_registry_add_listener(m_registry, &registry_listener, this);
  LOG_DEBUG("Performing Wayland roundtrip 1...");
  if (wl_display_roundtrip(m_display) == -1) {
    LOG_ERROR("Roundtrip 1 failed");
    return false;
  }
  LOG_DEBUG("Performing Wayland roundtrip 2...");
  if (wl_display_roundtrip(m_display) == -1) {
    LOG_ERROR("Roundtrip 2 failed");
    return false;
  }
  LOG_INFO("MonitorManager initialized.");
  return true;
#endif
}
void MonitorManager::update() {
#ifndef _WIN32
  if (m_display) {
    wl_display_dispatch(m_display);
  }
#endif
}
void MonitorManager::processPending() {
#ifndef _WIN32
  if (m_display) {
    wl_display_dispatch_pending(m_display);
  }
#endif
}
#ifndef _WIN32
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
#else
void MonitorManager::addMonitor(uint32_t id, wl_output *output) {}
void MonitorManager::removeMonitor(uint32_t id) {}
#endif
std::vector<MonitorInfo> MonitorManager::getMonitors() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<MonitorInfo> result;
#ifndef _WIN32
  for (const auto &[id, monitor] : m_monitors) {
    result.push_back(monitor->getInfo());
  }
#endif
  return result;
}
std::optional<MonitorInfo>
MonitorManager::getMonitor(const std::string &name) const {
  std::lock_guard<std::mutex> lock(m_mutex);
#ifndef _WIN32
  for (const auto &[id, monitor] : m_monitors) {
    if (monitor->getInfo().name == name) {
      return monitor->getInfo();
    }
  }
#endif
  return std::nullopt;
}
std::optional<MonitorInfo> MonitorManager::getPrimaryMonitor() const {
  std::lock_guard<std::mutex> lock(m_mutex);
#ifndef _WIN32
  if (!m_monitors.empty()) {
    return m_monitors.begin()->second->getInfo();
  }
#endif
  return std::nullopt;
}
void MonitorManager::setCallback(MonitorCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_callback = callback;
}
}  
