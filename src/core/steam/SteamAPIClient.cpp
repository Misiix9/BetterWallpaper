#include "SteamAPIClient.hpp"
#include "../utils/Logger.hpp"
#include <curl/curl.h>
#include <iostream>
#include <format>
#include <regex>

namespace bwp::steam {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

SteamAPIClient& SteamAPIClient::getInstance() {
    static SteamAPIClient instance;
    return instance;
}

SteamAPIClient::SteamAPIClient() {
    curl_global_init(CURL_GLOBAL_ALL);
}

SteamAPIClient::~SteamAPIClient() {
    curl_global_cleanup();
}

void SteamAPIClient::setApiKey(const std::string& key) {
    m_apiKey = key;
}

std::string SteamAPIClient::getApiKey() const {
    return m_apiKey;
}

bool SteamAPIClient::hasApiKey() const {
    return !m_apiKey.empty();
}

std::string SteamAPIClient::performRequest(const std::string& url, const std::string& postData) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "BetterWallpaper/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        if (!postData.empty()) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOG_ERROR("SteamAPIClient Request Failed: " + std::string(curl_easy_strerror(res)));
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

WorkshopItem SteamAPIClient::parseItem(const nlohmann::json& j) {
    WorkshopItem item;
    try {
        item.id = j.value("publishedfileid", "");
        item.title = j.value("title", "Untitled");
        item.thumbnailUrl = j.value("preview_url", "");
        item.author = ""; // Not always provided directly in search list
        item.votesUp = j.value("vote_data", nlohmann::json::object()).value("votes_up", 0);
        
        // Tags
        if (j.contains("tags")) {
            for (const auto& tag : j["tags"]) {
                if (tag.contains("tag")) {
                    item.tags.push_back(tag["tag"].get<std::string>());
                }
            }
        }
    } catch (...) {
        LOG_WARN("Failed to parse partial workshop item");
    }
    return item;
}

SearchResult SteamAPIClient::search(const std::string& query, int page, const std::string& sort) {
    SearchResult result;
    
    // IPublishedFileService/QueryFiles/v1/
    std::string baseUrl = "https://api.steampowered.com/IPublishedFileService/QueryFiles/v1/";
    
    // URL Encode query
    CURL* curl = curl_easy_init();
    std::string safeQuery;
    if(curl) {
        char* encodedQuery = curl_easy_escape(curl, query.c_str(), query.length());
        if(encodedQuery) {
            safeQuery = std::string(encodedQuery);
            curl_free(encodedQuery);
        }
        curl_easy_cleanup(curl);
    }
    
    std::string url = baseUrl + "?key=" + m_apiKey + 
                      "&appid=431960"
                      "&search_text=" + safeQuery + 
                      "&return_vote_data=true&return_tags=true&return_details=true&numperpage=50"
                      "&page=" + std::to_string(page);
                      
    if (sort == "ranked") {
        url += "&query_type=1"; // k_PublishedFileQueryType_RankedByVote
    } else if (sort == "recent") {
         url += "&query_type=9"; // k_PublishedFileQueryType_DateCreated
    } else {
         url += "&query_type=0"; // RankedByTextSearch
    }

    std::string jsonStr = performRequest(url);
    if (jsonStr.empty()) return result;
    
    // Check for HTML response (error page)
    if (jsonStr.find("<") == 0) {
        LOG_WARN("Steam API returned HTML (Access Denied).");
        return result;
    }

    try {
        auto j = nlohmann::json::parse(jsonStr);
        if (j.contains("response")) {
            auto& resp = j["response"];
            result.total = resp.value("total", 0);
            if (resp.contains("publishedfiledetails")) {
                for (const auto& item : resp["publishedfiledetails"]) {
                    result.items.push_back(parseItem(item));
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("JSON Parse Error (Search): " + std::string(e.what()));
    }

    return result;
}

std::vector<WorkshopItem> SteamAPIClient::getDetails(const std::vector<std::string>& ids) {
    // POST request to GetPublishedFileDetails
    std::vector<WorkshopItem> results;
    if (ids.empty()) return results;

    std::string url = "https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/";
    std::string postData = "itemcount=" + std::to_string(ids.size());
    for (size_t i = 0; i < ids.size(); ++i) {
        postData += "&publishedfileids[" + std::to_string(i) + "]=" + ids[i];
    }
    // Key is technically required for this one too usually
    // But this endpoint typically works via POST without key for public items?
    // Safer to add key if we have it
    // Wait, this is form-encoded data. curl separates fields
    
    // Correct way:
    // This endpoint accepts standard POST params.
    
    // Easier way: Use QueryFiles with specific IDs if possible? No.
    
    // Let's rely on search for now as it returns details.
    return results;
}
std::vector<WorkshopItem> SteamAPIClient::getUserFavorites(const std::string& steamId) {
    // Requires key
    if (m_apiKey.empty()) return {};
    // Not implemented fully yet
    return {};
}

}

