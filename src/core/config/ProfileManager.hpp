#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
namespace bwp::config {
class ProfileManager {
public:
  static ProfileManager &getInstance();
  void loadProfiles();
  std::vector<std::string> getProfileNames() const;
  nlohmann::json getProfile(const std::string &name) const;
  bool createProfile(const std::string &name, const nlohmann::json &content);
  bool updateProfile(const std::string &name, const nlohmann::json &content);
  bool deleteProfile(const std::string &name);
  bool duplicateProfile(const std::string &srcName, const std::string &newName);
  std::string getActiveProfileName() const;
  void setActiveProfile(const std::string &name);
private:
  ProfileManager();
  std::filesystem::path getProfilesDir() const;
  std::string m_activeProfile;
};
}  
