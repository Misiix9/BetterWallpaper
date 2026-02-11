#pragma once
#include <string>
#include <memory>
#include "SteamAPIClient.hpp"
#include "SteamCMDWorker.hpp"
#include "../config/ConfigManager.hpp"

namespace bwp::steam {

class SteamService {
public:
    static SteamService& getInstance();

    // Load from config
    void initialize();

    // Check availability
    bool hasSteamCMD() const;

    // Config helpers
    std::string getSteamUser() const;
    void setSteamUser(const std::string& user);
    
    std::string getApiKey() const;
    void setApiKey(const std::string& key);
    
    // Pass sorting/search to API client
    SearchResult search(const std::string& query, int page, const std::string& sort = "textsearch");

    // Login Action
    // Password is NOT stored, only used for session
    void login(const std::string& password, 
               std::function<void(bool success, const std::string&)> cb,
               std::function<void()> on2Fa);
               
    void submit2FA(const std::string& code,
                   std::function<void(bool success, const std::string& msg)> callback);
    
    // Download Action
    void downloadWallpaper(const std::string& workshopId,
                           std::function<void(float)> progress,
                           std::function<void(bool)> complete);

    void cancelDownload();

private:
    SteamService();
    ~SteamService() = default;

    SteamCMDWorker m_worker;
};

}
