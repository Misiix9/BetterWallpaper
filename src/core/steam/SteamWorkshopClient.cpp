#include "SteamWorkshopClient.hpp"
#include "../utils/Logger.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <thread>

namespace bwp::steam {

// Helper for curl write
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

SteamWorkshopClient &SteamWorkshopClient::getInstance() {
  static SteamWorkshopClient instance;
  return instance;
}

SteamWorkshopClient::SteamWorkshopClient() {
  curl_global_init(CURL_GLOBAL_ALL);
}

SteamWorkshopClient::~SteamWorkshopClient() { curl_global_cleanup(); }

bool SteamWorkshopClient::initialize() {
  // Check if steamcmd exists
  // Simple check: system("which steamcmd") or search path
  if (system("which steamcmd > /dev/null 2>&1") != 0) {
    LOG_ERROR("steamcmd not found in PATH.");
    return false;
  }
  return true;
}

std::string SteamWorkshopClient::makeCurlRequest(const std::string &url) {
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "BetterWallpaper/1.0");
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      LOG_ERROR("curl_easy_perform() failed: " +
                std::string(curl_easy_strerror(res)));
    }
    curl_easy_cleanup(curl);
  }
  return readBuffer;
}

void SteamWorkshopClient::search(const std::string &query, int page,
                                 SearchCallback callback) {
  // Note: Steam Workshop search API is unofficial/undocumented mostly without
  // key. Using a known endpoint or public scraping. For this prototype, we'll
  // try a public IPublishedFileService call if we had a key. Without API key,
  // it's harder. ALTERNATIVE: Use steamcmd or a different scraper.

  // Let's use a mocked search for the prototype if we don't have a reliable
  // open API. Real implementation would require parsing HTML from
  // steamcommunity.com or using a Web API key.

  // MOCK IMPLEMENTATION to proceed with structure validation
  // In a real app, you would hit:
  // https://api.steampowered.com/IPublishedFileService/QueryFiles/v1/

  std::thread([this, query, callback]() {
    std::vector<WorkshopItem> items;

    // Mock items
    WorkshopItem item1;
    item1.id = "123456789";
    item1.title = "Cyberpunk City - " + query;
    item1.author = "NeonArtist";
    item1.previewUrl =
        "https://steamuserimages-a.akamaihd.net/ugc/mock.jpg"; // Mock
    items.push_back(item1);

    WorkshopItem item2;
    item2.id = "987654321";
    item2.title = "Anime Scenery " + query;
    item2.author = "OtakuDev";
    items.push_back(item2);

    if (callback)
      callback(items);
  }).detach();
}

void SteamWorkshopClient::download(const std::string &workshopId,
                                   ProgressCallback progress,
                                   FinishCallback finish) {
  if (m_downloading) {
    if (finish)
      finish(false, "Download already in progress");
    return;
  }

  m_downloading = true;

  std::thread([this, workshopId, progress, finish]() {
    std::string cmd =
        "steamcmd +login anonymous +workshop_download_item 431960 " +
        workshopId + " +quit";
    LOG_INFO("Executing: " + cmd);

    // In real implementation, use popen to read output for progress
    int ret = system(cmd.c_str());

    m_downloading = false;

    if (ret == 0) {
      // Default location for steamcmd workshop content
      // ~/.local/share/Steam/steamapps/workshop/content/431960/<id> or similar
      // Depending on where steamcmd installs.

      // We need to return the path.
      std::string path = "";
      // Logic to find path... assuming default relative to home
      const char *home = std::getenv("HOME");
      if (home) {
        path = std::string(home) +
               "/.steam/steam/steamapps/workshop/content/431960/" + workshopId;
      }

      if (finish)
        finish(true, path);
    } else {
      if (finish)
        finish(false, "steamcmd failed");
    }
  }).detach();
}

} // namespace bwp::steam
