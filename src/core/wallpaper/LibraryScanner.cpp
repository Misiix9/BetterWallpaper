#include "LibraryScanner.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "WallpaperLibrary.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

namespace bwp::wallpaper {

LibraryScanner &LibraryScanner::getInstance() {
  static LibraryScanner instance;
  return instance;
}

LibraryScanner::LibraryScanner() {}

LibraryScanner::~LibraryScanner() {
  if (m_scanThread.joinable()) {
    m_scanThread.join();
  }
}

void LibraryScanner::setCallback(ScanCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_callback = callback;
}

void LibraryScanner::scan(const std::vector<std::string> &paths) {
  if (m_scanning)
    return;

  m_scanning = true;
  if (m_scanThread.joinable()) {
    m_scanThread.join();
  }

  m_scanThread = std::thread(&LibraryScanner::runScan, this, paths);
}

void LibraryScanner::runScan(std::vector<std::string> paths) {
  LOG_INFO("Starting library scan");

  std::string home = std::getenv("HOME");
  std::vector<std::string> workshopPaths = {
      home + "/.local/share/Steam/steamapps/workshop/content/431960",
      home + "/.steam/steam/steamapps/workshop/content/431960"};

  int totalFound = 0;

  // Scan Workshop Paths
  for (const auto &wsPathStr : workshopPaths) {
    std::filesystem::path wsPath(wsPathStr);
    if (!std::filesystem::exists(wsPath) ||
        !std::filesystem::is_directory(wsPath)) {
      LOG_INFO("Workshop path does not exist: " + wsPathStr);
      continue;
    }

    LOG_INFO("Scanning Workshop path: " + wsPath.string());

    int folderCount = 0;
    try {
      for (const auto &entry : std::filesystem::directory_iterator(wsPath)) {
        try {
          if (!entry.is_directory())
            continue;

          folderCount++;
          std::string folderId = entry.path().filename().string();
          LOG_INFO("Folder #" + std::to_string(folderCount) + ": " + folderId);

          // Try to add this workshop item
          bool added = scanWorkshopItem(entry.path());
          if (added) {
            totalFound++;
            LOG_INFO("  ADDED: " + folderId);
          } else {
            LOG_INFO("  SKIPPED: " + folderId);
          }
        } catch (const std::exception &e) {
          LOG_ERROR("Exception in folder loop: " + std::string(e.what()));
        }
      }
    } catch (const std::exception &e) {
      LOG_ERROR("Exception iterating workshop path: " + std::string(e.what()));
    }

    LOG_INFO("Processed " + std::to_string(folderCount) + " folders, found " +
             std::to_string(totalFound) + " items");
  }

  LOG_INFO("Workshop scan complete. Total: " + std::to_string(totalFound));

  // Scan User Paths (skip if workshop path)
  for (const auto &pathStr : paths) {
    if (pathStr.find("431960") != std::string::npos)
      continue;

    std::filesystem::path path = utils::FileUtils::expandPath(pathStr);
    if (!std::filesystem::exists(path))
      continue;

    LOG_INFO("Scanning user path: " + path.string());
    try {
      if (std::filesystem::is_directory(path)) {
        for (const auto &entry :
             std::filesystem::recursive_directory_iterator(path)) {
          if (entry.is_regular_file()) {
            scanFile(entry.path());
          }
        }
      }
    } catch (const std::exception &e) {
      LOG_ERROR("Error scanning path " + pathStr + ": " + e.what());
    }
  }

  m_scanning = false;

  auto &library = WallpaperLibrary::getInstance();
  auto allItems = library.getAllWallpapers();
  LOG_INFO("Scan finished. Library has " + std::to_string(allItems.size()) +
           " wallpapers");
}

bool LibraryScanner::scanWorkshopItem(const std::filesystem::path &dir) {
  std::filesystem::path projectJson = dir / "project.json";
  std::string folderId = dir.filename().string();

  WallpaperInfo info;
  info.id = folderId;
  info.source = "workshop";
  try {
    info.workshop_id = std::stoull(folderId);
  } catch (...) {
    info.workshop_id = 0;
  }

  // First, find ANY valid media file in the folder
  std::string foundFile;
  std::string foundPreview;

  try {
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file())
        continue;

      std::string filename = entry.path().filename().string();
      std::string ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

      // Check for preview
      if (filename.find("preview") != std::string::npos) {
        foundPreview = entry.path().string();
      }

      // Check for main file
      if (ext == ".mp4" || ext == ".webm" || ext == ".pkg") {
        foundFile = entry.path().string();
      } else if (foundFile.empty() && (ext == ".jpg" || ext == ".jpeg" ||
                                       ext == ".png" || ext == ".gif")) {
        // Only use image if no video/scene found
        if (filename.find("preview") == std::string::npos) {
          foundFile = entry.path().string();
        }
      }
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Error scanning folder " + folderId + ": " + e.what());
    return false;
  }

  // Try to get title from project.json
  std::string title = folderId;
  if (std::filesystem::exists(projectJson)) {
    try {
      std::ifstream f(projectJson);
      if (f.is_open()) {
        nlohmann::json j = nlohmann::json::parse(f);
        title = j.value("title", folderId);

        // If project.json has a file key, prefer that
        std::string jsonFile = j.value("file", "");
        if (!jsonFile.empty()) {
          std::filesystem::path fullPath = dir / jsonFile;
          if (std::filesystem::exists(fullPath)) {
            foundFile = fullPath.string();
          }
        }

        // Get type
        std::string type = j.value("type", "");
        if (type == "scene")
          info.type = WallpaperType::WEScene;
        else if (type == "video")
          info.type = WallpaperType::WEVideo;
        else if (type == "web")
          info.type = WallpaperType::WEWeb;
      }
    } catch (...) {
      // JSON parse failed, use defaults
    }
  }

  if (foundFile.empty()) {
    // No main file found, try preview as last resort
    if (!foundPreview.empty()) {
      foundFile = foundPreview;
    } else {
      return false;
    }
  }

  info.path = foundFile;

  // Determine type from extension if not set
  if (info.type == WallpaperType::Unknown) {
    std::string ext = std::filesystem::path(foundFile).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".pkg")
      info.type = WallpaperType::WEScene;
    else if (ext == ".mp4" || ext == ".webm")
      info.type = WallpaperType::Video;
    else
      info.type = WallpaperType::StaticImage;
  }

  auto &library = WallpaperLibrary::getInstance();
  library.addWallpaper(info);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_callback)
      m_callback(info.path);
  }

  return true;
}

void LibraryScanner::scanFile(const std::filesystem::path &path) {
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  bool isImage = (ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
                  ext == ".gif" || ext == ".bmp");
  bool isVideo =
      (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi");
  bool isScene = (ext == ".pkg");

  if (!isImage && !isVideo && !isScene)
    return;

  std::string id = path.string();

  auto &library = WallpaperLibrary::getInstance();
  if (library.getWallpaper(id))
    return;

  WallpaperInfo info;
  info.id = id;
  info.path = path.string();
  try {
    info.size_bytes = std::filesystem::file_size(path);
  } catch (...) {
    info.size_bytes = 0;
  }

  if (isScene)
    info.type = WallpaperType::WEScene;
  else if (isVideo)
    info.type = WallpaperType::Video;
  else
    info.type = WallpaperType::StaticImage;

  library.addWallpaper(info);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_callback)
      m_callback(path.string());
  }
}

} // namespace bwp::wallpaper
