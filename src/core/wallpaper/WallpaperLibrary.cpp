#include "WallpaperLibrary.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "../utils/StringUtils.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace bwp::wallpaper {

WallpaperLibrary &WallpaperLibrary::getInstance() {
  static WallpaperLibrary instance;
  return instance;
}

WallpaperLibrary::WallpaperLibrary() { m_dbPath = getDatabasePath(); }

WallpaperLibrary::~WallpaperLibrary() {
  if (m_dirty) {
    save();
  }
}

void WallpaperLibrary::initialize() { load(); }

std::filesystem::path WallpaperLibrary::getDatabasePath() const {
  const char *dataHome = std::getenv("XDG_DATA_HOME");
  std::filesystem::path dataDir;
  if (dataHome) {
    dataDir = std::filesystem::path(dataHome);
  } else {
    const char *home = std::getenv("HOME");
    if (home) {
      dataDir = std::filesystem::path(home) / ".local" / "share";
    } else {
      return "library.json";
    }
  }
  return dataDir / "betterwallpaper" / "library.json";
}

void WallpaperLibrary::load() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!std::filesystem::exists(m_dbPath))
    return;

  try {
    std::string content = utils::FileUtils::readFile(m_dbPath);
    nlohmann::json j = nlohmann::json::parse(content);

    m_wallpapers.clear();
    for (const auto &item : j["wallpapers"]) {
      // Deserialize WallpaperInfo
      WallpaperInfo info;
      info.id = item.value("id", "");
      info.path = item.value("path", "");
      info.type = static_cast<WallpaperType>(item.value("type", 0));
      // ... load other fields
      info.rating = item.value("rating", 0);
      info.favorite = item.value("favorite", false);
      info.play_count = item.value("play_count", 0);

      if (item.contains("added")) {
        info.added = std::chrono::system_clock::time_point(
            std::chrono::seconds(item["added"]));
      }
      if (item.contains("last_used")) {
        info.last_used = std::chrono::system_clock::time_point(
            std::chrono::seconds(item["last_used"]));
      }

      if (item.contains("tags")) {
        for (const auto &tag : item["tags"]) {
          info.tags.push_back(tag);
        }
      }

      // Load Settings
      if (item.contains("settings")) {
        const auto &s = item["settings"];
        info.settings.fps = s.value("fps", 0);
        info.settings.volume = s.value("volume", 100);
        info.settings.muted = s.value("muted", false);
        info.settings.playback_speed = s.value("playback_speed", 1.0f);
        info.settings.scaling =
            static_cast<ScalingMode>(s.value("scaling", 1)); // Default Fill
      }

      if (!info.id.empty()) {
        if (std::filesystem::exists(info.path)) {
          m_wallpapers[info.id] = info;
        } else {
          LOG_WARN("Removed missing wallpaper from library: " + info.path);
          m_dirty = true;
        }
      }
    }

    if (m_dirty) {
      save(); // Commit removal immediately
    }

    LOG_INFO("Loaded " + std::to_string(m_wallpapers.size()) +
             " wallpapers from library");
  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to load library: ") + e.what());
  }
}

void WallpaperLibrary::save() {
  std::lock_guard<std::mutex> lock(m_mutex);
  try {
    nlohmann::json j;
    j["wallpapers"] = nlohmann::json::array();

    for (const auto &pair : m_wallpapers) {
      const auto &info = pair.second;
      nlohmann::json item;
      item["id"] = info.id;
      item["path"] = info.path;
      item["type"] = static_cast<int>(info.type);
      item["rating"] = info.rating;
      item["favorite"] = info.favorite;
      item["play_count"] = info.play_count;
      item["added"] = std::chrono::duration_cast<std::chrono::seconds>(
                          info.added.time_since_epoch())
                          .count();
      item["last_used"] = std::chrono::duration_cast<std::chrono::seconds>(
                              info.last_used.time_since_epoch())
                              .count();
      item["tags"] = info.tags;

      // Save Settings
      nlohmann::json settings;
      settings["fps"] = info.settings.fps;
      settings["volume"] = info.settings.volume;
      settings["muted"] = info.settings.muted;
      settings["playback_speed"] = info.settings.playback_speed;
      settings["scaling"] = static_cast<int>(info.settings.scaling);
      item["settings"] = settings;

      j["wallpapers"].push_back(item);
    }

    utils::FileUtils::createDirectories(m_dbPath.parent_path());
    utils::FileUtils::writeFile(m_dbPath, j.dump(4));
    m_dirty = false;
    LOG_INFO("Saved library database");
  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to save library: ") + e.what());
  }
}

void WallpaperLibrary::addWallpaper(const WallpaperInfo &info) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_wallpapers[info.id] = info;
    m_dirty = true;
  }
  // Auto-save for local wallpapers to ensure persistence
  if (info.source == "local") {
    save();
  }
}

void WallpaperLibrary::updateWallpaper(const WallpaperInfo &info) {
  bool needsSave = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_wallpapers.count(info.id)) {
      m_wallpapers[info.id] = info;
      m_dirty = true;
      needsSave = true;
    }
  }
  // Call save outside the lock to avoid deadlock
  if (needsSave) {
    save();
  }
}

void WallpaperLibrary::removeWallpaper(const std::string &id) {
  bool needsSave = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_wallpapers.erase(id)) {
      m_dirty = true;
      needsSave = true;
    }
  }
  // Call save outside the lock to avoid deadlock
  if (needsSave) {
    save();
  }
}

std::optional<WallpaperInfo>
WallpaperLibrary::getWallpaper(const std::string &id) const {
  // Lock? map access needs lock if writers exist
  // Mutex is mutable
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_wallpapers.find(id);
  if (it != m_wallpapers.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<WallpaperInfo> WallpaperLibrary::getAllWallpapers() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<WallpaperInfo> result;
  result.reserve(m_wallpapers.size());
  for (const auto &pair : m_wallpapers) {
    result.push_back(pair.second);
  }
  return result;
}

std::vector<WallpaperInfo>
WallpaperLibrary::search(const std::string &query) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<WallpaperInfo> result;
  std::string q = utils::StringUtils::toLower(utils::StringUtils::trim(query));

  for (const auto &pair : m_wallpapers) {
    const auto &info = pair.second;
    // Search in path/name/tags
    bool match = false;
    std::string filename = std::filesystem::path(info.path).filename().string();

    if (utils::StringUtils::toLower(filename).find(q) != std::string::npos)
      match = true;

    for (const auto &tag : info.tags) {
      if (utils::StringUtils::toLower(tag).find(q) != std::string::npos)
        match = true;
    }

    if (match)
      result.push_back(info);
  }
  return result;
}

std::vector<WallpaperInfo> WallpaperLibrary::filter(
    const std::function<bool(const WallpaperInfo &)> &predicate) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<WallpaperInfo> result;
  for (const auto &pair : m_wallpapers) {
    if (predicate(pair.second)) {
      result.push_back(pair.second);
    }
  }
  return result;
}

std::vector<std::string> WallpaperLibrary::getAllTags() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<std::string> tags;
  for (const auto &pair : m_wallpapers) {
    for (const auto &tag : pair.second.tags) {
      bool found = false;
      for (const auto &existing : tags) {
        if (existing == tag) {
          found = true;
          break;
        }
      }
      if (!found) {
        tags.push_back(tag);
      }
    }
  }
  std::sort(tags.begin(), tags.end());
  return tags;
}

std::filesystem::path WallpaperLibrary::getDataDirectory() const {
  return m_dbPath.parent_path();
}

} // namespace bwp::wallpaper
