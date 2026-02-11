#include "SteamService.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SafeProcess.hpp"
#include "../utils/StringUtils.hpp"
#include "../wallpaper/WallpaperLibrary.hpp"
#include "../wallpaper/LibraryScanner.hpp"
#include "../utils/FileUtils.hpp"
#include <filesystem>
#include <vector>

namespace bwp::steam {

SteamService& SteamService::getInstance() {
    static SteamService instance;
    return instance;
}

SteamService::SteamService() {}

void SteamService::initialize() {
    auto& config = bwp::config::ConfigManager::getInstance();
    std::string stored = config.get<std::string>("steam_api_key", "");
    
    std::string key;
    if (!stored.empty()) {
        // Check if valid base64 (only contains A-Za-z0-9+/= chars)
        bool isBase64 = true;
        for (char c : stored) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '+' && c != '/' && c != '=') {
                isBase64 = false;
                break;
            }
        }
        
        if (isBase64 && stored.size() % 4 == 0) {
            // Likely base64 encoded — decode
            key = bwp::utils::StringUtils::base64Decode(stored);
        } else {
            // Legacy plaintext key — migrate to encoded
            key = bwp::utils::StringUtils::trim(stored);
            std::string encoded = bwp::utils::StringUtils::base64Encode(key);
            config.set("steam_api_key", encoded);
            LOG_INFO("Migrated plaintext API key to encoded storage");
        }
        key = bwp::utils::StringUtils::trim(key);
    }

    std::string user = config.get<std::string>("steam_user", "");
    
    SteamAPIClient::getInstance().setApiKey(key);
    LOG_INFO("Steam API key loaded: " + std::string(key.empty() ? "not set" : "present"));
}

bool SteamService::hasSteamCMD() const {
    return bwp::utils::SafeProcess::commandExists("steamcmd");
}

std::string SteamService::getSteamUser() const {
    return bwp::config::ConfigManager::getInstance().get<std::string>("steam_user", "");
}

void SteamService::setSteamUser(const std::string& user) {
    bwp::config::ConfigManager::getInstance().set("steam_user", user);
}

std::string SteamService::getApiKey() const {
    std::string stored = bwp::config::ConfigManager::getInstance().get<std::string>("steam_api_key", "");
    if (stored.empty()) return "";
    return bwp::utils::StringUtils::trim(bwp::utils::StringUtils::base64Decode(stored));
}

void SteamService::setApiKey(const std::string& key) {
    // Trim whitespace
    std::string cleanKey = bwp::utils::StringUtils::trim(key);
    
    // Encode as base64 for storage
    std::string encoded = cleanKey.empty() ? "" : bwp::utils::StringUtils::base64Encode(cleanKey);
    
    bwp::config::ConfigManager::getInstance().set("steam_api_key", encoded);
    SteamAPIClient::getInstance().setApiKey(cleanKey);
}

SearchResult SteamService::search(const std::string& query, int page, const std::string& sort) {
    return SteamAPIClient::getInstance().search(query, page, sort);
}

void SteamService::login(const std::string& password, 
                         std::function<void(bool success, const std::string&)> cb,
                         std::function<void()> on2Fa,
                         const std::string& twoFactorCode) {
    std::string user = getSteamUser();
    if (user.empty()) {
        if (cb) cb(false, "No Steam User configured");
        return;
    }
    
    m_worker.login(user, password, cb, on2Fa, twoFactorCode);
}

void SteamService::tryAutoLogin(std::function<void(bool success, const std::string& msg)> callback) {
    std::string user = getSteamUser();
    if (user.empty()) {
        if (callback) callback(false, "No Steam User configured");
        return;
    }
    
    m_worker.tryAutoLogin(user, callback);
}

void SteamService::submit2FA(const std::string& code,
                             std::function<void(bool success, const std::string& msg)> callback) {
    m_worker.submitTwoFactorCode(code, callback);
}

void SteamService::downloadWallpaper(const std::string& workshopId,
                                     std::function<void(float)> progress,
                                     std::function<void(bool)> complete) {
    
    m_worker.download(workshopId, progress, [this, workshopId, complete](bool success) {
        if (success) {
            // Trigger Library Reload
            LOG_INFO("Download complete for " + workshopId + ". Reloading library...");
            
            // Where does steamcmd put files?
            // Usually: ~/.local/share/Steam/steamapps/workshop/content/431960/<id>
            // or ~/Steam/...
            // The LibraryScanner scans these locations.
            // So calling reload() typically finds them.
            
            // Scan for the new item
            std::filesystem::path home = bwp::utils::FileUtils::getUserHomeDir();
            std::vector<std::filesystem::path> candidates = {
                home / ".local/share/Steam/steamapps/workshop/content/431960" / workshopId,
                home / "Steam/steamapps/workshop/content/431960" / workshopId,
                home / ".steam/steam/steamapps/workshop/content/431960" / workshopId
            };
            
            bool found = false;
            for(const auto& p : candidates) {
                if(std::filesystem::exists(p)) {
                    LOG_INFO("Found workshop item at " + p.string());
                    bwp::wallpaper::LibraryScanner::getInstance().scanWorkshopItem(p);
                    found = true;
                    break;
                }
            }
            if(!found) {
                LOG_WARN("Could not automatically locate downloaded item: " + workshopId);
            }
        }
        if (complete) complete(success);
    });
}

void SteamService::cancelDownload() {
    m_worker.cancel();
}

}
