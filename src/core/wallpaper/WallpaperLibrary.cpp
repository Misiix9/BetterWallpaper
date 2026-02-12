#include "WallpaperLibrary.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "../utils/StringUtils.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
namespace bwp::wallpaper {
WallpaperLibrary &WallpaperLibrary::getInstance() {
  static WallpaperLibrary instance;
  return instance;
}
WallpaperLibrary::WallpaperLibrary() {
  LOG_SCOPE_AUTO();
  m_dbPath = getDatabasePath();
}
WallpaperLibrary::~WallpaperLibrary() {
  if (m_dirty) {
    save();
  }
}
void WallpaperLibrary::initialize() {
  LOG_SCOPE_AUTO();
  if (m_initialized.exchange(true)) {
    return;
  }
  load();
}
std::filesystem::path WallpaperLibrary::getDatabasePath() const {
  LOG_SCOPE_AUTO();
  const char *dataHome = std::getenv("XDG_DATA_HOME");
  std::filesystem::path dataDir;
  if (dataHome) {
    dataDir = std::filesystem::path(dataHome);
  } else {
    const char *home = std::getenv("HOME");
    if (home) {
      dataDir = std::filesystem::path(home) / ".local" / "share";
    } else {
      return std::filesystem::current_path() / "library.json";
    }
  }
  return dataDir / "betterwallpaper" / "library.json";
}
void WallpaperLibrary::load() {
  LOG_SCOPE_AUTO();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!std::filesystem::exists(m_dbPath))
    return;
  try {
    std::string content = utils::FileUtils::readFile(m_dbPath);
    nlohmann::json j = nlohmann::json::parse(content);
    m_wallpapers.clear();
    for (const auto &item : j["wallpapers"]) {
      WallpaperInfo info;
      info.id = item.value("id", "");
      info.path = item.value("path", "");
      info.title = item.value("title", "");
      if (info.title.empty() && !info.path.empty()) {
        info.title = std::filesystem::path(info.path).stem().string();
        m_dirty = true;
      }
      info.type = static_cast<WallpaperType>(item.value("type", 0));
      info.source = item.value("source", std::string(""));
      info.rating = item.value("rating", 0);
      info.favorite = item.value("favorite", false);
      info.play_count = item.value("play_count", 0);
      if (item.contains("added")) {
        info.added = (long long)item["added"];
      }
      if (info.added == 0) {
        info.added =
            static_cast<long long>(std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()));
        m_dirty = true;
      }
      if (item.contains("last_used")) {
        info.last_used = (long long)item["last_used"];
      }
      if (item.contains("tags")) {
        for (const auto &tag : item["tags"]) {
          info.tags.push_back(tag);
        }
      }
      if (item.contains("settings")) {
        const auto &s = item["settings"];
        info.settings.fps = s.value("fps", -1);
        info.settings.volume = s.value("volume", -1);
        info.settings.muted = s.value("muted", false);
        info.settings.playback_speed = s.value("playback_speed", 1.0f);
        info.settings.scaling = static_cast<ScalingMode>(s.value("scaling", 0));
        info.settings.noAudioProcessing = s.value("no_audio_processing", -1);
        info.settings.disableMouse = s.value("disable_mouse", -1);
        info.settings.noAutomute = s.value("no_automute", -1);
      }
      info.blurhash = item.value("blurhash", "");
      if (!info.id.empty()) {
        if (std::filesystem::exists(info.path)) {
          m_wallpapers[info.id] = info;
        } else {
          LOG_WARN("Removed missing wallpaper from library: " + info.path);
          m_dirty = true;
        }
      }
    }
    LOG_INFO("Loaded " + std::to_string(m_wallpapers.size()) +
             " wallpapers from library");
    m_pathToId.clear();
    std::vector<std::string> idsToRemove;
    for (const auto &pair : m_wallpapers) {
      const auto &info = pair.second;
      std::string normPath = info.path;
      try {
        if (std::filesystem::exists(info.path)) {
          normPath = std::filesystem::canonical(info.path).string();
        }
      } catch (...) {
      }
      if (m_pathToId.count(normPath)) {
        std::string existingId = m_pathToId[normPath];
        const auto &existing = m_wallpapers[existingId];
        bool keepCurrent = false;
        if ((info.source == "workshop" || info.source == "steam") &&
            (existing.source != "workshop" && existing.source != "steam")) {
          keepCurrent = true;
        } else if ((existing.source == "workshop" ||
                    existing.source == "steam") &&
                   (info.source != "workshop" && info.source != "steam")) {
          keepCurrent = false;
        } else {
          if (info.id.length() < existing.id.length())
            keepCurrent = true;
        }
        if (keepCurrent) {
          idsToRemove.push_back(existingId);
          m_pathToId[normPath] = info.id;
        } else {
          idsToRemove.push_back(info.id);
        }
      } else {
        m_pathToId[normPath] = info.id;
      }
    }
    if (!idsToRemove.empty()) {
      LOG_INFO("Removing " + std::to_string(idsToRemove.size()) +
               " duplicate wallpapers.");
      for (const auto &id : idsToRemove) {
        m_wallpapers.erase(id);
      }
      m_dirty = true;
    }
  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to load library: ") + e.what());
  }
  if (m_dirty) {
    save();
  }
}
void WallpaperLibrary::save() {
  LOG_SCOPE_AUTO();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  try {
    nlohmann::json j;
    j["wallpapers"] = nlohmann::json::array();
    for (const auto &pair : m_wallpapers) {
      const auto &info = pair.second;
      nlohmann::json item;
      item["id"] = info.id;
      item["path"] = info.path;
      item["title"] = info.title;
      item["type"] = static_cast<int>(info.type);
      item["rating"] = info.rating;
      item["favorite"] = info.favorite;
      item["play_count"] = info.play_count;
      item["added"] = info.added;
      item["last_used"] = info.last_used;
      item["tags"] = info.tags;
      item["source"] = info.source;
      nlohmann::json settings;
      settings["fps"] = info.settings.fps;
      settings["volume"] = info.settings.volume;
      settings["muted"] = info.settings.muted;
      settings["playback_speed"] = info.settings.playback_speed;
      settings["scaling"] = static_cast<int>(info.settings.scaling);
      settings["no_audio_processing"] = info.settings.noAudioProcessing;
      settings["disable_mouse"] = info.settings.disableMouse;
      settings["no_automute"] = info.settings.noAutomute;
      item["settings"] = settings;
      if (!info.blurhash.empty()) {
        item["blurhash"] = info.blurhash;
      }
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
  LOG_SCOPE_AUTO();
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::string normPath = info.path;
    try {
      if (std::filesystem::exists(info.path))
        normPath = std::filesystem::canonical(info.path).string();
    } catch (...) {
    }
    if (m_pathToId.count(normPath)) {
      std::string existingId = m_pathToId[normPath];
      if (existingId != info.id) {
        LOG_INFO("Ignoring duplicate wallpaper import: " + info.path +
                 " (Existing ID: " + existingId + ")");
        return;
      }
      auto &existing = m_wallpapers[existingId];
      if (!info.title.empty())
        existing.title = info.title;
      if (info.type != WallpaperType::Unknown)
        existing.type = info.type;
      if (!info.source.empty())
        existing.source = info.source;
      if (!info.tags.empty())
        existing.tags = info.tags;
      m_dirty = true;
      return;
    }
    WallpaperInfo newInfo = info;
    if (newInfo.added == 0) {
      newInfo.added =
          static_cast<long long>(std::chrono::system_clock::to_time_t(
              std::chrono::system_clock::now()));
    }
    if (newInfo.title.empty()) {
      newInfo.title = std::filesystem::path(newInfo.path).stem().string();
    }
    m_wallpapers[newInfo.id] = newInfo;
    m_pathToId[normPath] = newInfo.id;
    m_dirty = true;
  }
  save();
  if (m_changeCallback) {
    m_changeCallback(info);
  }
}
void WallpaperLibrary::updateWallpaper(const WallpaperInfo &info) {
  LOG_SCOPE_AUTO();
  bool needsSave = false;
  ChangeCallback cb;
  std::vector<ChangeCallback> cbs;
  std::vector<ChangeCallback> idCbs;
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_wallpapers.count(info.id)) {
      m_wallpapers[info.id] = info;
      m_dirty = true;
      needsSave = true;
      cb = m_changeCallback;
      cbs = m_changeCallbacks;
      for (const auto &[id, icb] : m_idCallbacks) {
        idCbs.push_back(icb);
      }
    }
  }
  if (needsSave) {
    save();
    if (cb) {
      cb(info);
    }
    for (const auto &callback : cbs) {
      if (callback) {
        callback(info);
      }
    }
    for (const auto &callback : idCbs) {
      if (callback) {
        callback(info);
      }
    }
  }
}
void WallpaperLibrary::updateBlurhash(const std::string &id,
                                      const std::string &hash) {
  LOG_SCOPE_AUTO();
  bool needsSave = false;
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_wallpapers.find(id);
    if (it != m_wallpapers.end()) {
      it->second.blurhash = hash;
      m_dirty = true;
      needsSave = true;
    }
  }
  if (needsSave) {
    save();
  }
}
void WallpaperLibrary::removeWallpaper(const std::string &id) {
  LOG_SCOPE_AUTO();
  bool needsSave = false;
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_wallpapers.count(id)) {
      std::string path = m_wallpapers[id].path;
      m_wallpapers.erase(id);
      try {
        if (std::filesystem::exists(path)) {
          std::string norm = std::filesystem::canonical(path).string();
          if (m_pathToId.count(norm) && m_pathToId[norm] == id) {
            m_pathToId.erase(norm);
          }
        } else {
          for (auto it = m_pathToId.begin(); it != m_pathToId.end();) {
            if (it->second == id)
              it = m_pathToId.erase(it);
            else
              ++it;
          }
        }
      } catch (...) {
      }
      m_dirty = true;
      needsSave = true;
    }
  }
  if (needsSave) {
    save();
  }
}
void WallpaperLibrary::removeDuplicates() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
}
std::optional<WallpaperInfo>
WallpaperLibrary::getWallpaper(const std::string &id) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_wallpapers.find(id);
  if (it != m_wallpapers.end()) {
    return it->second;
  }
  return std::nullopt;
}
std::vector<WallpaperInfo> WallpaperLibrary::getAllWallpapers() const {
  LOG_SCOPE_AUTO();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<WallpaperInfo> result;
  result.reserve(m_wallpapers.size());
  for (const auto &pair : m_wallpapers) {
    result.push_back(pair.second);
  }
  return result;
}
std::vector<WallpaperInfo>
WallpaperLibrary::search(const std::string &query) const {
  LOG_SCOPE_AUTO();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<WallpaperInfo> result;
  std::string q = utils::StringUtils::toLower(utils::StringUtils::trim(query));
  for (const auto &pair : m_wallpapers) {
    const auto &info = pair.second;
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
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<WallpaperInfo> result;
  for (const auto &pair : m_wallpapers) {
    if (predicate(pair.second)) {
      result.push_back(pair.second);
    }
  }
  return result;
}
void WallpaperLibrary::setChangeCallback(ChangeCallback cb) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_changeCallback = cb;
}
void WallpaperLibrary::addChangeCallback(ChangeCallback cb) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_changeCallbacks.push_back(std::move(cb));
}
int WallpaperLibrary::addChangeCallbackWithId(ChangeCallback cb) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  int id = m_nextCallbackId++;
  m_idCallbacks[id] = std::move(cb);
  return id;
}
void WallpaperLibrary::removeChangeCallback(int id) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_idCallbacks.erase(id);
}
std::vector<std::string> WallpaperLibrary::getAllTags() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
