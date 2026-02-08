#include "ThumbnailCache.hpp"
#include "../utils/Blurhash.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <functional>
#ifndef _WIN32
#include <gtk/gtk.h>
#endif
#include <iomanip>
#include <sstream>
#include <thread>

namespace bwp::wallpaper {

ThumbnailCache &ThumbnailCache::getInstance() {
  static ThumbnailCache instance;
  return instance;
}

#ifndef _WIN32
ThumbnailCache::ThumbnailCache() { initCacheDir(); }

ThumbnailCache::~ThumbnailCache() {
  // Free all cached pixbufs
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &[key, entry] : m_memoryCache) {
    if (entry.pixbuf) {
      g_object_unref(entry.pixbuf);
    }
  }
  m_memoryCache.clear();
}

void ThumbnailCache::initCacheDir() {
  const char *cacheHome = std::getenv("XDG_CACHE_HOME");
  if (cacheHome && std::strlen(cacheHome) > 0) {
    m_cacheDir =
        std::filesystem::path(cacheHome) / "betterwallpaper" / "thumbnails";
  } else {
    const char *home = std::getenv("HOME");
    if (home) {
      m_cacheDir = std::filesystem::path(home) / ".cache" / "betterwallpaper" /
                   "thumbnails";
    } else {
      m_cacheDir = "/tmp/betterwallpaper/thumbnails";
    }
  }

  // Create directory if needed
  std::error_code ec;
  std::filesystem::create_directories(m_cacheDir, ec);
  if (ec) {
    LOG_WARN("Failed to create thumbnail cache directory: " +
             m_cacheDir.string() + " - " + ec.message());
  } else {
    LOG_INFO("Thumbnail cache directory: " + m_cacheDir.string());
  }
}

std::string
ThumbnailCache::generateCacheKey(const std::string &wallpaperPath) const {
  // Simple hash using std::hash
  std::hash<std::string> hasher;
  size_t hash = hasher(wallpaperPath);

  // Convert to hex string
  std::ostringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return ss.str();
}

std::filesystem::path
ThumbnailCache::getCachePath(const std::string &wallpaperPath,
                             Size size) const {
  std::string key = generateCacheKey(wallpaperPath);
  std::string filename =
      key + "_" + std::to_string(static_cast<int>(size)) + ".png";
  return m_cacheDir / filename;
}

bool ThumbnailCache::isCached(const std::string &wallpaperPath,
                              Size size) const {
  auto cachePath = getCachePath(wallpaperPath, size);
  return std::filesystem::exists(cachePath);
}

GdkPixbuf *ThumbnailCache::getSync(const std::string &wallpaperPath,
                                   Size size) {
  std::string key = generateCacheKey(wallpaperPath) + "_" +
                    std::to_string(static_cast<int>(size));

  // Check memory cache first
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_memoryCache.find(key);
    if (it != m_memoryCache.end()) {
      it->second.lastAccess = std::chrono::steady_clock::now();
      // Return with added reference
      return GDK_PIXBUF(g_object_ref(it->second.pixbuf));
    }
  }

  // Check disk cache
  auto cachePath = getCachePath(wallpaperPath, size);
  if (std::filesystem::exists(cachePath)) {
    GdkPixbuf *pixbuf = loadFromCache(cachePath);
    if (pixbuf) {
      // Add to memory cache
      std::lock_guard<std::mutex> lock(m_mutex);

      // Evict oldest if cache is full
      if (m_memoryCache.size() >= m_maxMemoryCacheEntries) {
        auto oldest =
            std::min_element(m_memoryCache.begin(), m_memoryCache.end(),
                             [](const auto &a, const auto &b) {
                               return a.second.lastAccess < b.second.lastAccess;
                             });
        if (oldest != m_memoryCache.end()) {
          if (oldest->second.pixbuf) {
            g_object_unref(oldest->second.pixbuf);
          }
          m_memoryCache.erase(oldest);
        }
      }

      m_memoryCache[key] = CacheEntry{GDK_PIXBUF(g_object_ref(pixbuf)),
                                      std::chrono::steady_clock::now()};

      return pixbuf;
    }
  }

  return nullptr;
}

GdkPixbuf *ThumbnailCache::generateSync(const std::string &wallpaperPath,
                                        Size size) {
  // Determine file type
  std::filesystem::path path(wallpaperPath);
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  GdkPixbuf *pixbuf = nullptr;

  bool isVideo =
      (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi");
  bool isImage = (ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
                  ext == ".gif" || ext == ".bmp" || ext == ".webp");
  bool isScene = (ext == ".pkg" || ext == ".json");

  if (isVideo || isScene) {
    // Videos and scene packages use preview images from the same folder
    pixbuf = generateFromVideo(wallpaperPath, size);
  } else if (isImage) {
    pixbuf = generateFromImage(wallpaperPath, size);
  }

  if (pixbuf) {
    // Save to disk cache
    auto cachePath = getCachePath(wallpaperPath, size);
    if (saveToCache(pixbuf, cachePath)) {
      LOG_DEBUG("Cached thumbnail: " + wallpaperPath);
    }

    // Add to memory cache
    std::string key = generateCacheKey(wallpaperPath) + "_" +
                      std::to_string(static_cast<int>(size));
    std::lock_guard<std::mutex> lock(m_mutex);

    // Evict oldest if full
    if (m_memoryCache.size() >= m_maxMemoryCacheEntries) {
      auto oldest =
          std::min_element(m_memoryCache.begin(), m_memoryCache.end(),
                           [](const auto &a, const auto &b) {
                             return a.second.lastAccess < b.second.lastAccess;
                           });
      if (oldest != m_memoryCache.end()) {
        if (oldest->second.pixbuf) {
          g_object_unref(oldest->second.pixbuf);
        }
        m_memoryCache.erase(oldest);
      }
    }

    m_memoryCache[key] = CacheEntry{GDK_PIXBUF(g_object_ref(pixbuf)),
                                    std::chrono::steady_clock::now()};
  }

  return pixbuf;
}

GdkPixbuf *ThumbnailCache::generateFromImage(const std::string &path,
                                             Size size) {
  GError *error = nullptr;
  int targetSize = static_cast<int>(size);

  // Load and scale in one operation for efficiency
  GdkPixbuf *pixbuf =
      gdk_pixbuf_new_from_file_at_scale(path.c_str(), targetSize, targetSize,
                                        TRUE, // preserve_aspect_ratio
                                        &error);

  if (error) {
    LOG_WARN("Failed to generate thumbnail for " + path + ": " +
             error->message);
    g_error_free(error);
    return nullptr;
  }

  return pixbuf;
}

GdkPixbuf *ThumbnailCache::generateFromVideo(const std::string &path,
                                             Size size) {
  // For videos, check for existing preview image first
  std::filesystem::path videoPath(path);
  std::filesystem::path previewPath = videoPath.parent_path() / "preview.jpg";

  if (std::filesystem::exists(previewPath)) {
    return generateFromImage(previewPath.string(), size);
  }

  // Try common preview file names
  std::vector<std::string> previewNames = {"preview.png", "preview.gif",
                                           "thumb.jpg", "thumbnail.jpg"};
  for (const auto &name : previewNames) {
    previewPath = videoPath.parent_path() / name;
    if (std::filesystem::exists(previewPath)) {
      return generateFromImage(previewPath.string(), size);
    }
  }

  // TODO: Use ffmpeg/gstreamer to extract frame
  // For now, return nullptr (fallback to placeholder)
  LOG_DEBUG("No preview found for video: " + path);
  return nullptr;
}

GdkPixbuf *
ThumbnailCache::loadFromCache(const std::filesystem::path &cachePath) {
  GError *error = nullptr;
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(cachePath.c_str(), &error);

  if (error) {
    LOG_WARN("Failed to load cached thumbnail: " + std::string(error->message));
    g_error_free(error);
    return nullptr;
  }

  return pixbuf;
}

bool ThumbnailCache::saveToCache(GdkPixbuf *pixbuf,
                                 const std::filesystem::path &cachePath) {
  if (!pixbuf)
    return false;

  GError *error = nullptr;

  // Ensure parent directory exists
  std::error_code ec;
  std::filesystem::create_directories(cachePath.parent_path(), ec);

  // Save as PNG for quality (could use WebP for smaller size if available)
  gboolean success =
      gdk_pixbuf_save(pixbuf, cachePath.c_str(), "png", &error, nullptr);

  if (error) {
    LOG_WARN("Failed to save thumbnail: " + std::string(error->message));
    g_error_free(error);
    return false;
  }

  return success == TRUE;
}

void ThumbnailCache::getAsync(const std::string &wallpaperPath, Size size,
                              ThumbnailCallback callback) {
  // First check if already cached (memory or disk)
  GdkPixbuf *cached = getSync(wallpaperPath, size);
  if (cached) {
    // Already cached - call callback directly (we're on main thread)
    if (callback) {
      callback(cached);
    }
    g_object_unref(cached);
    return;
  }

  // Not cached - generate in background thread for better performance
  // This prevents UI freezing when loading many thumbnails
  std::thread([this, wallpaperPath, size, callback]() {
    GdkPixbuf *generated = generateSync(wallpaperPath, size);

    // Schedule callback on main thread via g_idle_add
    struct CallbackData {
      ThumbnailCallback callback;
      GdkPixbuf *pixbuf;
    };
    CallbackData *data = new CallbackData{callback, generated};

    g_idle_add(
        [](gpointer userData) -> gboolean {
          CallbackData *d = static_cast<CallbackData *>(userData);
          if (d->callback) {
            d->callback(d->pixbuf);
          }
          if (d->pixbuf) {
            g_object_unref(d->pixbuf);
          }
          delete d;
          return G_SOURCE_REMOVE;
        },
        data);
  }).detach();
}

void ThumbnailCache::invalidate(const std::string &wallpaperPath) {
  // Remove from memory cache
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_memoryCache.begin(); it != m_memoryCache.end();) {
      if (it->first.find(generateCacheKey(wallpaperPath)) == 0) {
        if (it->second.pixbuf) {
          g_object_unref(it->second.pixbuf);
        }
        it = m_memoryCache.erase(it);
      } else {
        ++it;
      }
    }
  }

  // Remove from disk cache
  for (auto size : {Size::Small, Size::Medium, Size::Large}) {
    auto cachePath = getCachePath(wallpaperPath, size);
    std::error_code ec;
    std::filesystem::remove(cachePath, ec);
  }
}

void ThumbnailCache::clearCache() {
  // Clear memory cache
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &[key, entry] : m_memoryCache) {
      if (entry.pixbuf) {
        g_object_unref(entry.pixbuf);
      }
    }
    m_memoryCache.clear();
  }

  // Clear disk cache
  std::error_code ec;
  for (const auto &entry :
       std::filesystem::directory_iterator(m_cacheDir, ec)) {
    std::filesystem::remove(entry.path(), ec);
  }

  LOG_INFO("Thumbnail cache cleared");
}

ThumbnailCache::CacheStats ThumbnailCache::getStats() const {
  CacheStats stats{0, 0, 0};

  std::error_code ec;
  if (std::filesystem::exists(m_cacheDir)) {
    for (const auto &entry :
         std::filesystem::directory_iterator(m_cacheDir, ec)) {
      if (entry.is_regular_file()) {
        stats.cachedCount++;
        stats.totalSizeBytes += entry.file_size();
      }
    }
  }

  // Estimate memory usage
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &[key, entry] : m_memoryCache) {
      if (entry.pixbuf) {
        int width = gdk_pixbuf_get_width(entry.pixbuf);
        int height = gdk_pixbuf_get_height(entry.pixbuf);
        int channels = gdk_pixbuf_get_n_channels(entry.pixbuf);
        stats.memoryUsageBytes += width * height * channels;
      }
    }
  }

  return stats;
}

void ThumbnailCache::setMaxCacheSize(size_t megabytes) {
  m_maxDiskCacheMB = megabytes;
  pruneCache();
}

void ThumbnailCache::pruneCache() {
  size_t maxBytes = m_maxDiskCacheMB * 1024 * 1024;

  struct CacheFile {
    std::filesystem::path path;
    size_t size;
    std::filesystem::file_time_type lastWrite;
  };

  std::vector<CacheFile> files;
  size_t totalSize = 0;

  std::error_code ec;
  for (const auto &entry :
       std::filesystem::directory_iterator(m_cacheDir, ec)) {
    if (entry.is_regular_file()) {
      size_t fileSize = entry.file_size();
      files.push_back({entry.path(), fileSize, entry.last_write_time()});
      totalSize += fileSize;
    }
  }

  if (totalSize <= maxBytes) {
    return; // Under limit
  }

  // Sort by last write time (oldest first)
  std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) {
    return a.lastWrite < b.lastWrite;
  });

  // Delete oldest files until under limit
  for (const auto &file : files) {
    if (totalSize <= maxBytes)
      break;

    std::filesystem::remove(file.path, ec);
    if (!ec) {
      totalSize -= file.size;
      LOG_DEBUG("Pruned cached thumbnail: " + file.path.string());
    }
  }

  LOG_INFO("Thumbnail cache pruned to " +
           std::to_string(totalSize / (1024 * 1024)) + " MB");
}

std::string ThumbnailCache::computeBlurhash(const std::string &wallpaperPath,
                                            Size size) {
  // Get or generate the thumbnail
  GdkPixbuf *pixbuf = getSync(wallpaperPath, size);
  if (!pixbuf) {
    pixbuf = generateSync(wallpaperPath, size);
  }
  if (!pixbuf) {
    return {};
  }

  int width = gdk_pixbuf_get_width(pixbuf);
  int height = gdk_pixbuf_get_height(pixbuf);
  int channels = gdk_pixbuf_get_n_channels(pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  const guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

  // Blurhash encoder expects tightly-packed RGB (3 bytes per pixel)
  // GdkPixbuf may have alpha + rowstride padding, so we need to convert
  std::vector<uint8_t> rgb(width * height * 3);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const guchar *src = pixels + y * rowstride + x * channels;
      uint8_t *dst = rgb.data() + (y * width + x) * 3;
      dst[0] = src[0];
      dst[1] = src[1];
      dst[2] = src[2];
    }
  }

  g_object_unref(pixbuf);

  // 4x3 components is a good balance of quality vs string length
  std::string hash =
      bwp::utils::blurhash::encode(rgb.data(), width, height, 4, 3);

  if (!hash.empty()) {
    LOG_DEBUG("Computed blurhash for: " + wallpaperPath + " => " + hash);
  }
  return hash;
}

#else
// Windows Stubs
ThumbnailCache::ThumbnailCache() {}
ThumbnailCache::~ThumbnailCache() {}

GdkPixbuf* ThumbnailCache::getSync(const std::string&, Size) { return nullptr; }
GdkPixbuf* ThumbnailCache::generateSync(const std::string&, Size) { return nullptr; }
void ThumbnailCache::getAsync(const std::string&, Size, ThumbnailCallback) {}
bool ThumbnailCache::isCached(const std::string&, Size) const { return false; }
void ThumbnailCache::clearCache() {}
void ThumbnailCache::setMaxCacheSize(size_t) {}
ThumbnailCache::CacheStats ThumbnailCache::getStats() const { return {}; }
void ThumbnailCache::invalidate(const std::string&) {}
void ThumbnailCache::initCacheDir() {}
void ThumbnailCache::pruneCache() {}
std::string ThumbnailCache::generateCacheKey(const std::string& wallpaperPath) const { return ""; }
std::filesystem::path ThumbnailCache::getCachePath(const std::string&, Size) const { return ""; }
GdkPixbuf* ThumbnailCache::generateFromImage(const std::string&, Size) { return nullptr; }
GdkPixbuf* ThumbnailCache::generateFromVideo(const std::string&, Size) { return nullptr; }
GdkPixbuf* ThumbnailCache::loadFromCache(const std::filesystem::path&) { return nullptr; }
bool ThumbnailCache::saveToCache(GdkPixbuf*, const std::filesystem::path&) { return false; }
std::string ThumbnailCache::computeBlurhash(const std::string&, Size) { return ""; }
#endif

} // namespace bwp::wallpaper
