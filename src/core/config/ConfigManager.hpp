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
  static ConfigManager &getInstance(); // Singleton

  bool load();
  bool save();
  
  /**
   * @brief Schedule a debounced save (saves after a delay, coalescing rapid changes)
   * @param delayMs Delay in milliseconds before actual save (default 500ms)
   */
  void scheduleSave(int delayMs = 500);

  // Generic getters/setters
  // Generic getters/setters
  template <typename T> T get(const std::string &key) const;
  template <typename T>
  T get(const std::string &key, const T &defaultValue) const;

  template <typename T> void set(const std::string &key, const T &value);

  // Raw JSON access if needed
  nlohmann::json &getJson() { return m_config; }

  void resetToDefaults();

  // Signal listeners
  using Callback =
      std::function<void(const std::string &key, const nlohmann::json &value)>;
  void startWatching(Callback callback);

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
  
  // Debounced save mechanism
  std::atomic<guint> m_saveTimerId{0};
  std::atomic<bool> m_dirty{false};
};

// Template implementation must be in header
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

  // Fallback to defaults
  nlohmann::json defaults = SettingsSchema::getDefaults();
  // Traverse defaults with same key logic (simplified here)
  // For now returning default constructed T if not found
  return T{};
}

template <typename T>
T ConfigManager::get(const std::string &key, const T &defaultValue) const {
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
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    nlohmann::json *ptr = getValuePtr(key);
    if (ptr) {
      *ptr = value;
    }
  }
  
  // Use debounced save to avoid excessive disk writes
  scheduleSave();

  if (m_callback) {
    m_callback(key, value);
  }
}

} // namespace bwp::config
