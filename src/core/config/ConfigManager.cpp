#include "ConfigManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "../utils/StringUtils.hpp"
#include <fstream>
#include <iostream>
namespace bwp::config {
ConfigManager &ConfigManager::getInstance() {
  static ConfigManager instance;
  return instance;
}
ConfigManager::ConfigManager() {
  LOG_SCOPE_AUTO();
  m_configPath = getConfigPath();
  if (!load()) {
    resetToDefaults();
    save();
  }
}
std::filesystem::path ConfigManager::getConfigPath() const {
  LOG_SCOPE_AUTO();
  
  // adhere to XDG Base Directory spec where possible
  const char *configHome = std::getenv("XDG_CONFIG_HOME");
  std::filesystem::path configDir;
  if (configHome) {
    configDir = std::filesystem::path(configHome);
  } else {
    const char *home = std::getenv("HOME");
    if (home) {
      configDir = std::filesystem::path(home) / ".config";
    } else {
      // fallback relative (windows or weird linux setup?)
      return "config.json";  
    }
  }
  return configDir / "betterwallpaper" / "config.json";
}
bool ConfigManager::load() {
  LOG_SCOPE_AUTO();
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!std::filesystem::exists(m_configPath))
    return false;
  try {
    std::string content = utils::FileUtils::readFile(m_configPath);
    m_config = nlohmann::json::parse(content);
    
    // LOG_INFO("Loaded configuration: " + m_config.dump(4));
    
    // ensure we always have valid defaults even if user config is partial (migration)
    nlohmann::json defaults = SettingsSchema::getDefaults();
    defaults.update(m_config);
    m_config = defaults;
    return true;
  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to load config: ") + e.what());
    return false;
  }
}
bool ConfigManager::save() {
  LOG_SCOPE_AUTO();
  std::lock_guard<std::mutex> lock(m_mutex);
  try {
    return utils::FileUtils::writeFile(m_configPath, m_config.dump(4));
  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to save config: ") + e.what());
    return false;
  }
}
void ConfigManager::resetToDefaults() {
  LOG_SCOPE_AUTO();
  std::lock_guard<std::mutex> lock(m_mutex);
  m_config = SettingsSchema::getDefaults();
}
void ConfigManager::startWatching(Callback callback) { m_callback = callback; }
int ConfigManager::addListener(Callback callback) {
  int id = m_nextListenerId++;
  m_listeners[id] = std::move(callback);
  return id;
}
void ConfigManager::removeListener(int id) {
  m_listeners.erase(id);
}
void ConfigManager::scheduleSave(int delayMs) {
#ifndef _WIN32
  m_dirty = true;
  guint oldTimer = m_saveTimerId.exchange(0);
  if (oldTimer > 0) {
    g_source_remove(oldTimer);
  }
  guint newTimer = g_timeout_add(
      delayMs,
      [](gpointer userData) -> gboolean {
        ConfigManager *self = static_cast<ConfigManager *>(userData);
        if (self->m_dirty) {
          self->m_dirty = false;
          self->save();
          LOG_DEBUG("Config saved (debounced)");
        }
        self->m_saveTimerId = 0;
        return G_SOURCE_REMOVE;
      },
      this);
  m_saveTimerId = newTimer;
#else
  save();
#endif
}
nlohmann::json *ConfigManager::getValuePtr(const std::string &key) {
  std::vector<std::string> parts = utils::StringUtils::split(key, '.');
  if (parts.empty())
    return nullptr;
  nlohmann::json *current = &m_config;
  for (const auto &part : parts) {
    if (!current->contains(part)) {
      (*current)[part] = nlohmann::json::object();
    }
    current = &(*current)[part];
  }
  return current;
}
const nlohmann::json *ConfigManager::getValuePtr(const std::string &key) const {
  std::vector<std::string> parts = utils::StringUtils::split(key, '.');
  if (parts.empty())
    return nullptr;
  const nlohmann::json *current = &m_config;
  for (const auto &part : parts) {
    if (!current->contains(part))
      return nullptr;
    current = &(*current).at(part);
  }
  return current;
}
}  
