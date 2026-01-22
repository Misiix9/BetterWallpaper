#pragma once
#include <functional>
#include <future>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace bwp::steam {

struct WorkshopItem {
  std::string id;
  std::string title;
  std::string description;
  std::string previewUrl;
  std::string author;
  std::string
      fileUrl; // For direct download if possible, otherwise use steamcmd
  int votesUp = 0;
  int votesDown = 0;
  std::vector<std::string> tags;
};

class SteamWorkshopClient {
public:
  using SearchCallback = std::function<void(const std::vector<WorkshopItem> &)>;
  using ProgressCallback = std::function<void(double)>;
  using FinishCallback =
      std::function<void(bool success, const std::string &path)>;

  static SteamWorkshopClient &getInstance();

  // Initialize (check steamcmd existence)
  bool initialize();

  // Search Steam Workshop via Web API
  // AppID 431960 is Wallpaper Engine
  void search(const std::string &query, int page, SearchCallback callback);

  // Download item via steamcmd
  void download(const std::string &workshopId, ProgressCallback progress,
                FinishCallback finish);

private:
  SteamWorkshopClient();
  ~SteamWorkshopClient();

  std::string makeCurlRequest(const std::string &url);

  std::atomic<bool> m_downloading{false};
  std::mutex m_mutex;
};

} // namespace bwp::steam
