#pragma once
#include "../config/ConfigManager.hpp"
#include "SteamAPIClient.hpp"
#include "SteamCMDWorker.hpp"
#include <memory>
#include <string>

namespace bwp::steam {

class SteamService {
public:
  static SteamService &getInstance();

  // Load from config
  void initialize();

  // Check availability
  bool hasSteamCMD() const;

  // Config helpers
  std::string getSteamUser() const;
  void setSteamUser(const std::string &user);

  std::string getApiKey() const;
  void setApiKey(const std::string &key);

  // Pass sorting/search to API client
  SearchResult search(const std::string &query, int page,
                      const std::string &sort = "textsearch");

  // Login Action
  // Password is NOT stored, only used for session
  // If twoFactorCode is non-empty, it is sent with the login attempt.
  void login(const std::string &password,
             std::function<void(bool success, const std::string &)> cb,
             std::function<void()> on2Fa,
             const std::string &twoFactorCode = "");

  // Auto-login: try using steamcmd's cached session (no password needed)
  void tryAutoLogin(
      std::function<void(bool success, const std::string &msg)> callback);

  void
  submit2FA(const std::string &code,
            std::function<void(bool success, const std::string &msg)> callback);

  // Download Action
  void downloadWallpaper(const std::string &workshopId,
                         const std::string &title,
                         const std::string &thumbnailUrl,
                         const std::string &author, int votesUp,
                         std::function<void(float)> progress,
                         std::function<void(bool)> complete);

  void cancelDownload();

private:
  SteamService();
  ~SteamService() = default;

  SteamCMDWorker m_worker;
};

} // namespace bwp::steam
