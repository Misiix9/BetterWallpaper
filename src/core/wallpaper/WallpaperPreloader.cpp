#include "WallpaperPreloader.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/Logger.hpp"
#include "renderers/StaticRenderer.hpp"
#include "renderers/VideoRenderer.hpp"
#include "renderers/WallpaperEngineRenderer.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>

namespace bwp::wallpaper {

WallpaperPreloader &WallpaperPreloader::getInstance() {
  static WallpaperPreloader instance;
  return instance;
}

WallpaperPreloader::WallpaperPreloader() {
  LOG_INFO("WallpaperPreloader initialized");
}

WallpaperPreloader::~WallpaperPreloader() {
  // Cancel all ongoing preloads
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &[path, entry] : m_preloads) {
    entry->cancelled = true;
    if (entry->preloadThread.joinable()) {
      entry->preloadThread.detach();
    }
  }
  m_preloads.clear();
}

void WallpaperPreloader::preload(const std::string &path,
                                 PreloadCallback callback) {
  if (path.empty()) {
    LOG_WARN("Cannot preload empty path");
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  // Check if already preloaded or preloading
  auto it = m_preloads.find(path);
  if (it != m_preloads.end()) {
    auto &entry = it->second;
    if (entry->state == PreloadState::Ready) {
      LOG_DEBUG("Wallpaper already preloaded: " + path);
      if (callback) {
        callback(path, PreloadState::Ready);
      }
      return;
    }
    if (entry->state == PreloadState::Loading) {
      LOG_DEBUG("Wallpaper already preloading: " + path);
      // Update callback if provided
      if (callback) {
        entry->callback = callback;
      }
      return;
    }
  }

  // Evict old preloads if at limit
  evictOldPreloads();

  // Create new preload entry
  auto entry = std::make_shared<PreloadEntry>();
  entry->path = path;
  entry->state = PreloadState::Loading;
  entry->callback = callback;
  entry->preloadedAtMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count();

  m_preloads[path] = entry;

  LOG_INFO("Starting preload for: " + path);

  // Start preload in background thread
  entry->preloadThread = std::thread([this, path]() { doPreload(path); });
  entry->preloadThread.detach();
}

void WallpaperPreloader::doPreload(const std::string &path) {
  std::shared_ptr<PreloadEntry> entry;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_preloads.find(path);
    if (it == m_preloads.end()) {
      return;
    }
    entry = it->second;
  }

  if (entry->cancelled) {
    return;
  }

  // Detect wallpaper type
  WallpaperType type = detectType(path);

  std::shared_ptr<WallpaperRenderer> renderer;

  try {
    switch (type) {
    case WallpaperType::StaticImage: {
      LOG_DEBUG("Preloading static image: " + path);
      auto staticRenderer = std::make_shared<StaticRenderer>();
      if (staticRenderer->load(path)) {
        renderer = staticRenderer;
        LOG_INFO("Static image preloaded: " + path);
      }
      break;
    }

    case WallpaperType::Video: {
      LOG_DEBUG("Preloading video: " + path);
      auto videoRenderer = std::make_shared<VideoRenderer>();
      // Load but don't start playing
      if (videoRenderer->load(path)) {
        videoRenderer->pause(); // Ensure paused
        renderer = videoRenderer;
        LOG_INFO("Video preloaded (paused): " + path);
      }
      break;
    }

    case WallpaperType::WEScene:
    case WallpaperType::WEVideo:
    case WallpaperType::WEWeb: {
      LOG_DEBUG("Validating WE assets for: " + path);
      // For WE scenes, we can't really preload the external process
      // But we can validate that the assets exist
      auto weRenderer = std::make_shared<WallpaperEngineRenderer>();

      // Check if the path and required files exist
      std::filesystem::path scenePath(path);
      bool valid = false;

      if (std::filesystem::exists(scenePath)) {
        // Check for scene.json or project.json
        auto dir = scenePath.parent_path();
        if (std::filesystem::exists(dir / "scene.json") ||
            std::filesystem::exists(dir / "project.json") ||
            scenePath.extension() == ".pkg") {
          valid = true;
        }
      }

      if (valid) {
        // Store the renderer for later use
        // Note: We don't call load() here as that starts the process
        renderer = weRenderer;
        LOG_INFO("WE scene validated and ready: " + path);
      } else {
        LOG_WARN("WE scene validation failed: " + path);
      }
      break;
    }

    default:
      LOG_WARN("Unknown wallpaper type for preload: " + path);
      break;
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Preload failed for " + path + ": " + e.what());
  }

  // Update entry with result
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_preloads.find(path);
    if (it != m_preloads.end() && !it->second->cancelled) {
      it->second->renderer = renderer;
      it->second->state =
          renderer ? PreloadState::Ready : PreloadState::Failed;

      // Call callback on main thread
      PreloadCallback callback = it->second->callback;
      PreloadState state = it->second->state;

      if (callback) {
        // Use g_idle_add to call on main thread
        struct CallbackData {
          PreloadCallback cb;
          std::string path;
          PreloadState state;
        };
        CallbackData *data = new CallbackData{callback, path, state};

        g_idle_add(
            [](gpointer userData) -> gboolean {
              CallbackData *d = static_cast<CallbackData *>(userData);
              if (d->cb) {
                d->cb(d->path, d->state);
              }
              delete d;
              return G_SOURCE_REMOVE;
            },
            data);
      }
    }
  }
}

WallpaperPreloader::PreloadState
WallpaperPreloader::getState(const std::string &path) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_preloads.find(path);
  if (it != m_preloads.end()) {
    return it->second->state;
  }
  return PreloadState::None;
}

bool WallpaperPreloader::isReady(const std::string &path) const {
  return getState(path) == PreloadState::Ready;
}

std::shared_ptr<WallpaperRenderer>
WallpaperPreloader::getPreloadedRenderer(const std::string &path) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_preloads.find(path);
  if (it != m_preloads.end() && it->second->state == PreloadState::Ready) {
    auto renderer = it->second->renderer;
    // Remove from cache (will be owned by caller now)
    m_preloads.erase(it);
    return renderer;
  }
  return nullptr;
}

void WallpaperPreloader::clearPreload(const std::string &path) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (path.empty()) {
    // Clear all
    for (auto &[p, entry] : m_preloads) {
      entry->cancelled = true;
    }
    m_preloads.clear();
    LOG_INFO("Cleared all preloads");
  } else {
    auto it = m_preloads.find(path);
    if (it != m_preloads.end()) {
      it->second->cancelled = true;
      m_preloads.erase(it);
      LOG_DEBUG("Cleared preload for: " + path);
    }
  }
}

void WallpaperPreloader::cancelPreload(const std::string &path) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_preloads.find(path);
  if (it != m_preloads.end()) {
    it->second->cancelled = true;
    it->second->state = PreloadState::None;
    LOG_DEBUG("Cancelled preload for: " + path);
  }
}

void WallpaperPreloader::setMaxPreloads(size_t max) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_maxPreloads = max;
  evictOldPreloads();
}

void WallpaperPreloader::evictOldPreloads() {
  // Already under lock when called

  if (m_preloads.size() < m_maxPreloads) {
    return;
  }

  // Find oldest ready preloads to evict
  std::vector<std::pair<int64_t, std::string>> readyPreloads;
  for (const auto &[path, entry] : m_preloads) {
    if (entry->state == PreloadState::Ready) {
      readyPreloads.emplace_back(entry->preloadedAtMs, path);
    }
  }

  // Sort by time (oldest first)
  std::sort(readyPreloads.begin(), readyPreloads.end());

  // Evict oldest until under limit
  size_t toEvict = m_preloads.size() - m_maxPreloads + 1;
  for (size_t i = 0; i < toEvict && i < readyPreloads.size(); ++i) {
    const std::string &pathToEvict = readyPreloads[i].second;
    m_preloads.erase(pathToEvict);
    LOG_DEBUG("Evicted old preload: " + pathToEvict);
  }
}

WallpaperType WallpaperPreloader::detectType(const std::string &path) const {
  std::string mime = utils::FileUtils::getMimeType(path);

  if (mime.find("image/") != std::string::npos &&
      mime.find("gif") == std::string::npos) {
    return WallpaperType::StaticImage;
  } else if (mime.find("video/") != std::string::npos ||
             mime.find("gif") != std::string::npos) {
    return WallpaperType::Video;
  } else if (mime.find("x-wallpaper-engine") != std::string::npos) {
    return WallpaperType::WEScene;
  }

  // Fallback based on extension
  std::string ext = utils::FileUtils::getExtension(path);
  if (ext == "mp4" || ext == "webm" || ext == "mkv" || ext == "gif") {
    return WallpaperType::Video;
  }
  if (ext == "pkg" || ext == "json" || ext == "html" || ext == "htm") {
    return WallpaperType::WEScene;
  }

  return WallpaperType::StaticImage; // Default
}

} // namespace bwp::wallpaper
