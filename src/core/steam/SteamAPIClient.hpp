#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace bwp::steam {

struct WorkshopItem {
    std::string id;
    std::string title;
    std::string description;
    std::string thumbnailUrl;
    std::string previewUrl; // Larger image or video preview
    std::string author;
    int votesUp = 0;
    int subscriptions = 0;
    std::vector<std::string> tags;
    bool isDownloaded = false;
};

struct SearchResult {
    std::vector<WorkshopItem> items;
    int total = 0;  // Total matching items (for pagination)
};

class SteamAPIClient {
public:
    static SteamAPIClient& getInstance();

    // Set user API Key
    void setApiKey(const std::string& key);
    std::string getApiKey() const;
    bool hasApiKey() const;

    // Search workshop (AppId 431960)
    // sort can be: "texsearch" (Relevance), "ranked" (Popularity), "recent" (Date)
    SearchResult search(const std::string& query, int page = 1, const std::string& sort = "textsearch");

    // Get details for specific items
    std::vector<WorkshopItem> getDetails(const std::vector<std::string>& ids);

    // Collection retrieval (if user logs in or provides SteamID)
    std::vector<WorkshopItem> getUserFavorites(const std::string& steamId);

private:
    SteamAPIClient();
    ~SteamAPIClient();
    
    std::string m_apiKey;
    
    std::string performRequest(const std::string& url, const std::string& postData = "");
    
    // Helper to parse JSON response to WorkshopItem
    WorkshopItem parseItem(const nlohmann::json& j);
};

}
