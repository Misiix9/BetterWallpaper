#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
namespace bwp::steam {
enum class WorkshopItemType {
  Scene,  
  Video,  
  Web,    
  Unknown
};
enum class WorkshopSort { Popular, Recent, Trending, TopRated, Subscribed };
struct SearchFilters {
  WorkshopSort sort = WorkshopSort::Popular;
  std::string
      resolution;  
  WorkshopItemType type = WorkshopItemType::Scene;
  std::string timePeriod;  
  std::vector<std::string> tags;
};
struct WorkshopItem {
  std::string id;
  std::string title;
  std::string description;
  std::string previewUrl;
  std::string author;
  std::string authorId;
  std::string fileUrl;
  WorkshopItemType type = WorkshopItemType::Scene;
  int votesUp = 0;
  int votesDown = 0;
  int subscriberCount = 0;
  double rating = 0.0;  
  int64_t fileSize = 0;
  int64_t createdTime = 0;
  int64_t updatedTime = 0;
  std::vector<std::string> tags;
  bool isSupported() const {
    return type == WorkshopItemType::Scene || type == WorkshopItemType::Video;
  }
};
struct SearchResult {
  std::vector<WorkshopItem> items;
  int totalResults = 0;
  int currentPage = 0;
  int totalPages = 0;
  std::string nextCursor;  
};
struct DownloadProgress {
  std::string workshopId;
  std::string title;
  double progress = 0.0;  
  int64_t bytesDownloaded = 0;
  int64_t totalBytes = 0;
  bool isPaused = false;
  bool isCancelled = false;
};
enum class DownloadError {
  Success,
  SteamCmdMissing,
  LicenseRequired,
  NotFound,
  NetworkError,
  Unknown
};
struct DownloadResult {
  DownloadError error = DownloadError::Unknown;
  std::string path;        
  std::string message;     
  std::string installCmd;  
};
class SteamWorkshopClient {
public:
  using SearchCallback = std::function<void(const SearchResult &result)>;
  using ProgressCallback =
      std::function<void(const DownloadProgress &progress)>;
  using FinishCallback = std::function<void(const DownloadResult &result)>;
  static SteamWorkshopClient &getInstance();
  bool initialize();
  bool isSteamCmdAvailable() const;
  std::string getInstallCommand() const;
  std::string getDistroName() const;
  void setApiKey(const std::string &key) { m_apiKey = key; }
  bool hasApiKey() const { return !m_apiKey.empty(); }
  void search(const std::string &query, const SearchFilters &filters, int page,
              SearchCallback callback);
  void search(const std::string &query, int page,
              std::function<void(const std::vector<WorkshopItem> &)> callback);
  void download(const std::string &workshopId, ProgressCallback progress,
                FinishCallback finish);
  void cancelDownload();
  bool isDownloading() const { return m_downloading; }
  void login(const std::string &username) {
    m_username = username;
  }
  void logout() { m_username.clear(); }
  bool isLoggedIn() const { return !m_username.empty(); }
  std::string getUsername() const { return m_username; }
private:
  SteamWorkshopClient();
  ~SteamWorkshopClient();
  std::string makeCurlRequest(const std::string &url,
                              const std::string &postData = "");
  std::string makeApiUrl(const std::string &endpoint) const;
  SearchResult parseSearchResponse(const nlohmann::json &response);
  WorkshopItem parseWorkshopItem(const nlohmann::json &itemJson);
  WorkshopItemType parseItemType(const std::string &typeStr);
  std::string m_apiKey;
  std::string m_username;
  std::atomic<bool> m_downloading{false};
  std::atomic<bool> m_cancelRequested{false};
  std::mutex m_mutex;
  static constexpr int WALLPAPER_ENGINE_APPID = 431960;
  static constexpr const char *STEAM_API_BASE = "https://api.steampowered.com";
};
}  
