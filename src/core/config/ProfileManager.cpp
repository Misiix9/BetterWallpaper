#include "ProfileManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "ConfigManager.hpp"

namespace bwp::config {

ProfileManager &ProfileManager::getInstance() {
  static ProfileManager instance;
  return instance;
}

ProfileManager::ProfileManager() {
  loadProfiles();
  m_activeProfile =
      ConfigManager::getInstance().get<std::string>("current_profile");
}

std::filesystem::path ProfileManager::getProfilesDir() const {
  const char *configHome = std::getenv("XDG_CONFIG_HOME");
  std::filesystem::path configDir;
  if (configHome) {
    configDir = std::filesystem::path(configHome);
  } else {
    const char *home = std::getenv("HOME");
    if (home) {
      configDir = std::filesystem::path(home) / ".config";
    } else {
      return "profiles";
    }
  }
  return configDir / "betterwallpaper" / "profiles";
}

void ProfileManager::loadProfiles() {
  std::filesystem::path profilesDir = getProfilesDir();
  utils::FileUtils::createDirectories(profilesDir);

  // Check if default exists, if not create it
  if (!utils::FileUtils::exists(profilesDir / "default.json")) {
    nlohmann::json defaultProfile = {{"name", "default"},
                                     {"monitors", nlohmann::json::object()},
                                     {"triggers", nlohmann::json::array()}};
    createProfile("default", defaultProfile);
  }
}

std::vector<std::string> ProfileManager::getProfileNames() const {
  std::vector<std::string> names;
  std::filesystem::path profilesDir = getProfilesDir();
  if (!std::filesystem::exists(profilesDir))
    return names;

  for (const auto &entry : std::filesystem::directory_iterator(profilesDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      names.push_back(entry.path().stem().string());
    }
  }
  return names;
}

nlohmann::json ProfileManager::getProfile(const std::string &name) const {
  std::filesystem::path path = getProfilesDir() / (name + ".json");
  if (!utils::FileUtils::exists(path))
    return nlohmann::json{};

  try {
    return nlohmann::json::parse(utils::FileUtils::readFile(path));
  } catch (...) {
    return nlohmann::json{};
  }
}

bool ProfileManager::createProfile(const std::string &name,
                                   const nlohmann::json &content) {
  std::filesystem::path path = getProfilesDir() / (name + ".json");
  try {
    return utils::FileUtils::writeFile(path, content.dump(4));
  } catch (...) {
    return false;
  }
}

bool ProfileManager::updateProfile(const std::string &name,
                                   const nlohmann::json &content) {
  return createProfile(name, content);
}

bool ProfileManager::deleteProfile(const std::string &name) {
  if (name == "default")
    return false; // Cannot delete default
  std::filesystem::path path = getProfilesDir() / (name + ".json");
  return std::filesystem::remove(path);
}

bool ProfileManager::duplicateProfile(const std::string &srcName,
                                      const std::string &newName) {
  nlohmann::json src = getProfile(srcName);
  if (src.empty())
    return false;

  src["name"] = newName;
  return createProfile(newName, src);
}

std::string ProfileManager::getActiveProfileName() const {
  return m_activeProfile;
}

void ProfileManager::setActiveProfile(const std::string &name) {
  if (name == m_activeProfile)
    return;

  // Validate existence
  if (!utils::FileUtils::exists(getProfilesDir() / (name + ".json")))
    return;

  m_activeProfile = name;
  ConfigManager::getInstance().set<std::string>("current_profile", name);
  // Logic to apply profile will be handled by Daemon watching Config/Signal
}

} // namespace bwp::config
