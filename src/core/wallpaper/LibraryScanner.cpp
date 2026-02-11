#include "LibraryScanner.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "WallpaperLibrary.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
namespace bwp::wallpaper {
#ifdef _WIN32
#define G_SOURCE_REMOVE 0
inline void g_idle_add(void *func, void *data) {
  if (func) {
    using CallbackType = int (*)(void *);
    ((CallbackType)func)(data);
  }
}
#endif
struct ProgressData {
  ScanProgress progress;
  LibraryScanner::ProgressCallback callback;
};
struct CompletionData {
  int totalFound;
  LibraryScanner::CompletionCallback callback;
};
struct FileFoundData {
  std::string path;
  LibraryScanner::ScanCallback callback;
};
LibraryScanner &LibraryScanner::getInstance() {
  static LibraryScanner instance;
  return instance;
}
LibraryScanner::LibraryScanner() {}
LibraryScanner::~LibraryScanner() {
  cancelScan();
  if (m_scanThread.joinable()) {
    m_scanThread.join();
  }
}
void LibraryScanner::setCallback(ScanCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_callback = callback;
}
void LibraryScanner::setProgressCallback(ProgressCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_progressCallback = callback;
}
void LibraryScanner::setCompletionCallback(CompletionCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_completionCallback = callback;
}
ScanProgress LibraryScanner::getProgress() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_progress;
}
void LibraryScanner::cancelScan() { m_cancelRequested = true; }
void LibraryScanner::waitForCompletion() {
  if (m_scanThread.joinable()) {
    m_scanThread.join();
  }
}
void LibraryScanner::scan(const std::vector<std::string> &paths) {
  if (m_scanning)
    return;
  m_cancelRequested = false;
  m_scanning = true;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_progress = ScanProgress{};
  }
  if (m_scanThread.joinable()) {
    m_scanThread.join();
  }
  m_scanThread = std::thread(&LibraryScanner::runScan, this, paths);
}
gboolean LibraryScanner::idleProgress(gpointer data) {
  auto *pd = static_cast<ProgressData *>(data);
  if (pd && pd->callback) {
    pd->callback(pd->progress);
  }
  delete pd;
  return G_SOURCE_REMOVE;
}
gboolean LibraryScanner::idleCompletion(gpointer data) {
  auto *cd = static_cast<CompletionData *>(data);
  if (cd && cd->callback) {
    cd->callback(cd->totalFound);
  }
  delete cd;
  return G_SOURCE_REMOVE;
}
gboolean LibraryScanner::idleFileFound(gpointer data) {
  auto *fd = static_cast<FileFoundData *>(data);
  if (fd && fd->callback) {
    fd->callback(fd->path);
  }
  delete fd;
  return G_SOURCE_REMOVE;
}
void LibraryScanner::reportProgress() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_progressCallback) {
    auto *data = new ProgressData{m_progress, m_progressCallback};
    g_idle_add(idleProgress, data);
  }
}
void LibraryScanner::notifyCompletion() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_completionCallback) {
    auto *data =
        new CompletionData{m_progress.filesFound, m_completionCallback};
    g_idle_add(idleCompletion, data);
  }
}
void LibraryScanner::runScan(std::vector<std::string> paths) {
  LOG_INFO("Starting library scan");
  const char* homeEnv = std::getenv("HOME");
  if (!homeEnv) {
    LOG_ERROR("HOME environment variable is not set — skipping Steam Workshop scan");
  }
  std::string home = homeEnv ? homeEnv : "";
  std::vector<std::string> workshopPaths;
  if (!home.empty()) {
    workshopPaths = {
        home + "/.local/share/Steam/steamapps/workshop/content/431960",
        home + "/.steam/steam/steamapps/workshop/content/431960"};
  }
  int totalFound = 0;
  for (const auto &wsPathStr : workshopPaths) {
    if (m_cancelRequested) {
      LOG_INFO("Scan cancelled by user");
      break;
    }
    std::filesystem::path wsPath(wsPathStr);
    if (!std::filesystem::exists(wsPath) ||
        !std::filesystem::is_directory(wsPath)) {
      continue;
    }
    std::string canonicalWsPath;
    try {
      canonicalWsPath = std::filesystem::canonical(wsPath).string();
    } catch (...) {
      canonicalWsPath = wsPathStr;
    }
    static std::vector<std::string> scannedRoots;
    bool alreadyScanned = false;
    for (const auto &root : scannedRoots) {
      if (root == canonicalWsPath)
        alreadyScanned = true;
    }
    if (alreadyScanned) {
      LOG_INFO("Skipping duplicate workshop root: " + wsPathStr);
      continue;
    }
    scannedRoots.push_back(canonicalWsPath);
    LOG_INFO("Scanning Workshop path: " + wsPath.string());
    int folderCount = 0;
    try {
      for (const auto &entry : std::filesystem::directory_iterator(wsPath)) {
        if (m_cancelRequested)
          break;
        try {
          if (!entry.is_directory())
            continue;
          folderCount++;
          std::string folderId = entry.path().filename().string();
          {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_progress.filesScanned++;
            m_progress.currentPath = entry.path().string();
          }
          if (folderCount % 10 == 0) {
            reportProgress();
          }
          bool added = scanWorkshopItem(entry.path());
          if (added) {
            totalFound++;
            {
              std::lock_guard<std::mutex> lock(m_mutex);
              m_progress.filesFound = totalFound;
            }
            LOG_DEBUG("  ADDED: " + folderId);
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
  if (!m_cancelRequested) {
    LOG_INFO("Workshop scan complete. Total: " + std::to_string(totalFound));
    for (const auto &pathStr : paths) {
      if (m_cancelRequested)
        break;
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
            if (m_cancelRequested)
              break;
            if (entry.is_regular_file()) {
              {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_progress.filesScanned++;
                m_progress.currentPath = entry.path().string();
              }
              scanFile(entry.path());
            }
          }
        }
      } catch (const std::exception &e) {
        LOG_ERROR("Error scanning path " + pathStr + ": " + e.what());
      }
    }
  }
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_progress.isComplete = true;
    m_progress.filesFound = totalFound;
  }
  m_scanning = false;
  auto &library = WallpaperLibrary::getInstance();
  library.save();
  auto allItems = library.getAllWallpapers();
  LOG_INFO("Scan finished. Library has " + std::to_string(allItems.size()) +
           " wallpapers");
  notifyCompletion();
  reportProgress();
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
  std::string foundFile;
  std::string foundPreview;
  try {
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file())
        continue;
      std::string filename = entry.path().filename().string();
      std::string ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      if (filename.find("preview") != std::string::npos) {
        foundPreview = entry.path().string();
      }
      if (ext == ".mp4" || ext == ".webm" || ext == ".pkg" || ext == ".html") {
        foundFile = entry.path().string();
      } else if (foundFile.empty() && (ext == ".jpg" || ext == ".jpeg" ||
                                       ext == ".png" || ext == ".gif")) {
        if (filename.find("preview") == std::string::npos) {
          foundFile = entry.path().string();
        }
      }
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Error scanning folder " + folderId + ": " + e.what());
    return false;
  }
  std::string title = folderId;
  if (std::filesystem::exists(projectJson)) {
    try {
      std::ifstream f(projectJson);
      if (f.is_open()) {
        nlohmann::json j = nlohmann::json::parse(f);
        title = j.value("title", folderId);
        std::string jsonFile = j.value("file", "");
        if (!jsonFile.empty()) {
          std::filesystem::path fullPath = dir / jsonFile;
          if (std::filesystem::exists(fullPath)) {
            foundFile = fullPath.string();
          }
        }
        std::string type = j.value("type", "");
        if (type == "scene")
          info.type = WallpaperType::WEScene;
        else if (type == "video")
          info.type = WallpaperType::WEVideo;
        else if (type == "web")
          info.type = WallpaperType::WEWeb;
        if (j.contains("tags")) {
          auto tagsJson = j["tags"];
          if (tagsJson.is_array()) {
            info.tags = tagsJson.get<std::vector<std::string>>();
          }
        }
      }
    } catch (...) {
    }
  }
  if (foundFile.empty()) {
    if (!foundPreview.empty()) {
      foundFile = foundPreview;
    } else {
      return false;
    }
  }
  info.path = foundFile;
  info.title = title;
  if (info.type == WallpaperType::Unknown) {
    std::string ext = std::filesystem::path(foundFile).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".pkg")
      info.type = WallpaperType::WEScene;
    else if (ext == ".mp4" || ext == ".webm")
      info.type = WallpaperType::Video;
    else if (ext == ".html" || ext == ".htm")
      info.type = WallpaperType::WEWeb;
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
  bool isWeb = (ext == ".html" || ext == ".htm");
  if (!isImage && !isVideo && !isScene && !isWeb)
    return;
  std::string id = path.string();
  auto &library = WallpaperLibrary::getInstance();
  if (path.string().find("steamapps/workshop") != std::string::npos ||
      path.string().find("431960") != std::string::npos) {
    return;  
  }
  if (library.getWallpaper(id))
    return;
  WallpaperInfo info;
  info.id = id;
  info.source = "local";  
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
  else if (isWeb)
    info.type = WallpaperType::WEWeb;
  else
    info.type = WallpaperType::StaticImage;
  library.addWallpaper(info);
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_callback)
      m_callback(path.string());
  }
}
}  
