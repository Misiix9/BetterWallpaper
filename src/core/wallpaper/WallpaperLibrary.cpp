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
    return; // Already initialized, don't re-load and wipe in-memory state
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
      // Deserialize WallpaperInfo
      WallpaperInfo info;
      info.id = item.value("id", "");
      info.path = item.value("path", "");
      info.title = item.value("title", "");
      // Backfill title from filename if not set
      if (info.title.empty() && !info.path.empty()) {
        info.title = std::filesystem::path(info.path).stem().string();
        m_dirty = true;
      }
      info.type = static_cast<WallpaperType>(item.value("type", 0));
      // ... load other fields
      info.source = item.value("source", std::string(""));
      info.rating = item.value("rating", 0);
      info.favorite = item.value("favorite", false);
      info.play_count = item.value("play_count", 0);

      if (item.contains("added")) {
        // Warning: This expects seconds precision.
        info.added = (long long)item["added"]; // It is a long long in struct
      }
      // Backfill timestamp for wallpapers that were saved without one
      if (info.added == 0) {
        info.added = static_cast<long long>(
            std::chrono::system_clock::to_time_t(
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

      // Progressive loading placeholder
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

    // Perform robust deduplication
    // We must release lock before calling removeDuplicates if it takes a lock,
    // BUT we are already inside a method that took a lock.
    // removeDuplicates should NOT take a lock if called internally,
    // OR we should release here.
    // However, removeDuplicates is now implemented to take a lock in its
    // definition below. BUT we are holding m_mutex! This will deadlock if we
    // call it naively. So we need to put it outside the lock block? No, load()
    // finishes holding the lock. I will call removeDuplicatesInternal() here
    // which does NOT take a lock.

    // Actually, I can just unlock here?
    // No, m_wallpapers access must be protected.
    // Let's implement dedupe inline here to be safe and simple.

    // Use member m_pathToId
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
        // Prefer Workshop/Steam source
        if ((info.source == "workshop" || info.source == "steam") &&
            (existing.source != "workshop" && existing.source != "steam")) {
          keepCurrent = true;
        } else if ((existing.source == "workshop" ||
                    existing.source == "steam") &&
                   (info.source != "workshop" && info.source != "steam")) {
          keepCurrent = false;
        } else {
          // Heuristic: Shorter ID usually means custom folder name or simple ID
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
        // No duplicate found
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

  // Save dirty state (from backfilling titles/timestamps, deduplication, etc.)
  // Safe to call now because we use a recursive_mutex
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
      item["source"] = info.source; // Make sure to save source!

      // Save Settings
      nlohmann::json settings;
      settings["fps"] = info.settings.fps;
      settings["volume"] = info.settings.volume;
      settings["muted"] = info.settings.muted;
      settings["playback_speed"] = info.settings.playback_speed;
      settings["scaling"] = static_cast<int>(info.settings.scaling);
      item["settings"] = settings;

      // Progressive loading placeholder
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

    // Check for duplicates
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
      // Same path, same ID — merge scanner metadata but preserve user data
      auto &existing = m_wallpapers[existingId];
      if (!info.title.empty())
        existing.title = info.title;
      if (info.type != WallpaperType::Unknown)
        existing.type = info.type;
      if (!info.source.empty())
        existing.source = info.source;
      if (!info.tags.empty())
        existing.tags = info.tags;
      // Preserve: favorite, rating, play_count, added, last_used, settings, blurhash
      m_dirty = true;
      return;
    }

    // New wallpaper — set timestamp if not already set
    WallpaperInfo newInfo = info;
    if (newInfo.added == 0) {
      newInfo.added = static_cast<long long>(
          std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    }
    // Set title from filename if not provided (for local wallpapers)
    if (newInfo.title.empty()) {
      newInfo.title = std::filesystem::path(newInfo.path).stem().string();
    }

    m_wallpapers[newInfo.id] = newInfo;
    m_pathToId[normPath] = newInfo.id;
    m_dirty = true;
  }
  // Always save to ensure favorites/ratings and new wallpapers persist
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
  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_wallpapers.count(info.id)) {
      m_wallpapers[info.id] = info;
      m_dirty = true;
      needsSave = true;
      cb = m_changeCallback;
      cbs = m_changeCallbacks;
    }
  }
  // Call save and callbacks outside the lock to avoid deadlock
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

      // Clean up path index
      // Need to find which path mapped to this ID (reverse lookup or
      // re-canonicalize) Canonicalizing again is safest
      try {
        if (std::filesystem::exists(path)) {
          std::string norm = std::filesystem::canonical(path).string();
          if (m_pathToId.count(norm) && m_pathToId[norm] == id) {
            m_pathToId.erase(norm);
          }
        } else {
          // Fallback: iterate (slow but rare)
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
  // Call save outside the lock to avoid deadlock
  if (needsSave) {
    save();
  }
}

void WallpaperLibrary::removeDuplicates() {
  // Public wrapper if needed, but logic is primarily in load()
  // Re-run logic
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  // Copy-paste of dedupe logic if we want it callable at runtime
  // For now, load match is sufficient.
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
