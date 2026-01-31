#include "FolderManager.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "WallpaperLibrary.hpp"
#include <algorithm>
#include <random>

namespace bwp::wallpaper {

FolderManager &FolderManager::getInstance() {
  static FolderManager instance;
  return instance;
}

FolderManager::FolderManager() { load(); }

FolderManager::~FolderManager() { save(); }

void FolderManager::load() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto &lib = WallpaperLibrary::getInstance();
  std::filesystem::path path = lib.getDataDirectory() / "folders.json";

  if (!std::filesystem::exists(path))
    return;

  try {
    std::string content = utils::FileUtils::readFile(path);
    nlohmann::json j = nlohmann::json::parse(content);

    m_folders.clear();
    for (const auto &item : j["folders"]) {
      Folder f;
      f.id = item.value("id", "");
      f.name = item.value("name", "Unnamed");
      f.icon = item.value("icon", "folder-symbolic");
      if (item.contains("wallpapers")) {
        f.wallpaperIds = item["wallpapers"].get<std::vector<std::string>>();
      }
      if (!f.id.empty()) {
        m_folders[f.id] = f;
      }
    }
  } catch (...) {
  }
}

void FolderManager::save() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto &lib = WallpaperLibrary::getInstance();
  std::filesystem::path path =
      lib.getDataDirectory() / "folders.json"; // Store alongside library

  try {
    nlohmann::json j;
    j["folders"] = nlohmann::json::array();
    for (const auto &pair : m_folders) {
      const auto &f = pair.second;
      nlohmann::json item;
      item["id"] = f.id;
      item["name"] = f.name;
      item["icon"] = f.icon;
      item["wallpapers"] = f.wallpaperIds;
      j["folders"].push_back(item);
    }

    utils::FileUtils::writeFile(path, j.dump(4));
  } catch (...) {
  }
}

std::string FolderManager::createFolder(const std::string &name) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  Folder f;
  // Generate UUID or simpler ID
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);
  std::string id = "";
  const char *hex = "0123456789abcdef";
  for (int i = 0; i < 32; i++)
    id += hex[dis(gen)];

  f.id = id;
  f.name = name;
  m_folders[id] = f;
  save();
  return id;
}

void FolderManager::deleteFolder(const std::string &id) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_folders.erase(id)) {
    save();
  }
}

void FolderManager::renameFolder(const std::string &id,
                                 const std::string &newName) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_folders.count(id)) {
    m_folders[id].name = newName;
    save();
  }
}

void FolderManager::addToFolder(const std::string &folderId,
                                const std::string &wallpaperId) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_folders.count(folderId)) {
    auto &ids = m_folders[folderId].wallpaperIds;
    if (std::find(ids.begin(), ids.end(), wallpaperId) == ids.end()) {
      ids.push_back(wallpaperId);
      save();
    }
  }
}

void FolderManager::reorderWallpaper(const std::string &folderId,
                                     const std::string &wallpaperId,
                                     const std::string &targetId, bool after) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_folders.count(folderId)) {
    auto &ids = m_folders[folderId].wallpaperIds;

    // Find source
    auto sourceIt = std::find(ids.begin(), ids.end(), wallpaperId);
    if (sourceIt == ids.end())
      return;

    // Remove source
    ids.erase(sourceIt);

    // Find target
    auto targetIt = std::find(ids.begin(), ids.end(), targetId);
    if (targetIt == ids.end()) {
      // Target lost (?), append to end
      ids.push_back(wallpaperId);
    } else {
      if (after) {
        targetIt++; // Insert after
      }
      ids.insert(targetIt, wallpaperId);
    }
    save();
  }
}

std::vector<Folder> FolderManager::getFolders() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<Folder> list;
  for (const auto &pair : m_folders) {
    list.push_back(pair.second);
  }
  return list;
}

std::optional<Folder> FolderManager::getFolder(const std::string &id) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_folders.count(id))
    return m_folders.at(id);
  return std::nullopt;
}

} // namespace bwp::wallpaper
