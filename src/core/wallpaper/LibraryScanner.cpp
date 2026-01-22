#include "LibraryScanner.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "WallpaperLibrary.hpp"
#include <iostream>

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
    return; // Already scanning

  m_scanning = true;
  if (m_scanThread.joinable()) {
    m_scanThread.join();
  }

  m_scanThread = std::thread(&LibraryScanner::runScan, this, paths);
}

void LibraryScanner::runScan(std::vector<std::string> paths) {
  LOG_INFO("Starting library scan");

  // Create list of existing files to detect deletions later?
  // For now, just add new ones.

  for (const auto &pathStr : paths) {
    std::filesystem::path path = utils::FileUtils::expandPath(pathStr);
    if (!std::filesystem::exists(path))
      continue;

    try {
      if (std::filesystem::is_directory(path)) {
        for (const auto &entry :
             std::filesystem::recursive_directory_iterator(path)) {
          if (entry.is_regular_file()) {
            scanFile(entry.path());
          }
        }
      } else {
        scanFile(path);
      }
    } catch (const std::exception &e) {
      LOG_ERROR("Error scanning path " + pathStr + ": " + e.what());
    }
  }

  m_scanning = false;
  LOG_INFO("Library scan finished");
}

void LibraryScanner::scanFile(const std::filesystem::path &path) {
  // Check extension
  std::string mime = utils::FileUtils::getMimeType(path);
  bool isImage = mime.find("image/") != std::string::npos;
  bool isVideo = mime.find("video/") != std::string::npos;

  if (!isImage && !isVideo)
    return;

  // Hash check to see if already in library?
  // WallpaperLibrary needs getByHash or check existance.
  // For now, ID is hash.
  std::string hash = utils::FileUtils::calculateHash(path);

  auto &library = WallpaperLibrary::getInstance();
  if (library.getWallpaper(hash)) {
    // Exists, maybe update path if moved?
    // Skip for now
    return;
  }

  // Create info
  WallpaperInfo info;
  info.id = hash;
  info.path = path.string();
  info.hash = hash;
  info.size_bytes = std::filesystem::file_size(path);

  if (isVideo)
    info.type = WallpaperType::Video;
  else
    info.type = WallpaperType::StaticImage;

  // Resolution?? Need external tool or image/video library reader.
  // We can use GdkPixbuf for image dimensions in GUI thread/if linked GTK, or
  // specific tool. In core, we might not want GTK dep. relying on
  // ffmpeg/metadata reader maybe? Or just defer resolution loading to when
  // used.

  library.addWallpaper(info);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_callback)
      m_callback(path.string());
  }
}

} // namespace bwp::wallpaper
