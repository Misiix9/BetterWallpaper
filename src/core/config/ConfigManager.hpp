#pragma once
#include "SettingsSchema.hpp"
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include "../utils/Logger.hpp"

namespace bwp::config {

class ConfigManager {
public:
  static ConfigManager &getInstance(); // Singleton

  bool load();
  bool save();

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
  save();

  if (m_callback) {
    m_callback(key, value);
  }
}

} // namespace bwp::config
