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
  m_configPath = getConfigPath();
  if (!load()) {
    resetToDefaults();
    save();
  }
}

std::filesystem::path ConfigManager::getConfigPath() const {
  const char *configHome = std::getenv("XDG_CONFIG_HOME");
  std::filesystem::path configDir;
  if (configHome) {
    configDir = std::filesystem::path(configHome);
  } else {
    const char *home = std::getenv("HOME");
    if (home) {
      configDir = std::filesystem::path(home) / ".config";
    } else {
      return "config.json"; // Fallback
    }
  }
  return configDir / "betterwallpaper" / "config.json";
}

bool ConfigManager::load() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!std::filesystem::exists(m_configPath))
    return false;

  try {
    std::string content = utils::FileUtils::readFile(m_configPath);
    m_config = nlohmann::json::parse(content);

    // Merge with defaults to ensure all keys exist (migration)
    nlohmann::json defaults = SettingsSchema::getDefaults();
    // Start with defaults, update with loaded config so saved values take
    // precedence
    defaults.update(m_config);
    m_config = defaults;

    return true;
  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to load config: ") + e.what());
    return false;
  }
}

bool ConfigManager::save() {
  std::lock_guard<std::mutex> lock(m_mutex);
  try {
    // Pretty print with indent 4
    return utils::FileUtils::writeFile(m_configPath, m_config.dump(4));
  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to save config: ") + e.what());
    return false;
  }
}

void ConfigManager::resetToDefaults() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_config = SettingsSchema::getDefaults();
}

void ConfigManager::startWatching(Callback callback) { m_callback = callback; }

nlohmann::json *ConfigManager::getValuePtr(const std::string &key) {
  // Handle dot notation "general.autostart"
  std::vector<std::string> parts = utils::StringUtils::split(key, '.');
  if (parts.empty())
    return nullptr;

  nlohmann::json *current = &m_config;
  for (const auto &part : parts) {
    if (!current->contains(part)) {
      // Should create?
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

} // namespace bwp::config
