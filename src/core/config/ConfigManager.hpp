#pragma once
#include "SettingsSchema.hpp"
#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include "../utils/Logger.hpp"
#ifndef _WIN32
#include <gtk/gtk.h>
#endif
namespace bwp::config {
class ConfigManager {
public:
  static ConfigManager &getInstance();  
  bool load();
  bool save();
  void scheduleSave(int delayMs = 500);
  template <typename T> T get(const std::string &key) const;
  template <typename T>
  T get(const std::string &key, const T &defaultValue) const;
  template <typename T> void set(const std::string &key, const T &value);
  nlohmann::json &getJson() { return m_config; }
  void resetToDefaults();
  using Callback =
      std::function<void(const std::string &key, const nlohmann::json &value)>;
  void startWatching(Callback callback);
  // Add an additional listener for config changes (supports multiple)
  int addListener(Callback callback);
  void removeListener(int id);
private:
  ConfigManager();
  ~ConfigManager() = default;
  ConfigManager(const ConfigManager &) = delete;
  ConfigManager &operator=(const ConfigManager &) = delete;
  std::filesystem::path getConfigPath() const;
  nlohmann::json *getValuePtr(const std::string &key);
  const nlohmann::json *getValuePtr(const std::string &key) const;
  mutable std::mutex m_mutex;
  nlohmann::json m_config;
  std::filesystem::path m_configPath;
  Callback m_callback;
  std::map<int, Callback> m_listeners;
  int m_nextListenerId = 1;
  std::atomic<guint> m_saveTimerId{0};
  std::atomic<bool> m_dirty{false};
};
template <typename T> T ConfigManager::get(const std::string &key) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  const nlohmann::json *ptr = getValuePtr(key);
  if (ptr) {
    try {
      return ptr->get<T>();
    } catch (const std::exception &e) {
      LOG_ERROR("Config get error for key " + key + ": " + e.what());
    }
  }
  nlohmann::json defaults = SettingsSchema::getDefaults();
  return T{};
}
template <typename T>
T ConfigManager::get(const std::string &key, const T &defaultValue) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  const nlohmann::json *ptr = getValuePtr(key);
  if (ptr) {
    try {
      return ptr->get<T>();
    } catch (const std::exception &e) {
      LOG_ERROR("Config get error for key " + key + ": " + e.what());
    }
  }
  return defaultValue;
}
template <typename T>
void ConfigManager::set(const std::string &key, const T &value) {
  Callback callbackCopy;
  std::map<int, Callback> listenersCopy;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    nlohmann::json *ptr = getValuePtr(key);
    if (ptr) {
      *ptr = value;
    }
    callbackCopy = m_callback;
    listenersCopy = m_listeners;
  }
  scheduleSave();
  if (callbackCopy) {
    callbackCopy(key, value);
  }
  for (const auto &[id, listener] : listenersCopy) {
    if (listener) {
      listener(key, value);
    }
  }
}
}  
