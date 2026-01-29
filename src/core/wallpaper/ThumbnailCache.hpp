#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <mutex>
#include <string>
#include <unordered_map>

namespace bwp::wallpaper {

/**
 * @brief Persistent thumbnail cache system
 *
 * Features:
 * - Stores thumbnails as files in ~/.cache/betterwallpaper/thumbnails/
 * - Uses wallpaper path hash as cache key
 * - Supports multiple thumbnail sizes (128, 256, 512)
 * - LRU eviction when cache exceeds size limit
 * - Thread-safe operations
 * - Async thumbnail generation and loading
 */
class ThumbnailCache {
public:
  static ThumbnailCache &getInstance();

  /**
   * @brief Thumbnail size presets
   */
  enum class Size {
    Small = 128,  // Grid thumbnails (compact view)
    Medium = 256, // Grid thumbnails (normal view)
    Large = 512   // Preview panel
  };

  /**
   * @brief Get a cached thumbnail if available
   * @param wallpaperPath Path to the wallpaper file
   * @param size Desired thumbnail size
   * @return Cached thumbnail pixbuf or nullptr if not cached
   */
  GdkPixbuf *getSync(const std::string &wallpaperPath, Size size);

  /**
   * @brief Request a thumbnail asynchronously
   * @param wallpaperPath Path to the wallpaper file
   * @param size Desired thumbnail size
   * @param callback Called on main thread when thumbnail is ready
   */
  using ThumbnailCallback = std::function<void(GdkPixbuf *pixbuf)>;
  void getAsync(const std::string &wallpaperPath, Size size,
                ThumbnailCallback callback);

  /**
   * @brief Generate and cache a thumbnail synchronously
   * @param wallpaperPath Path to the wallpaper file
   * @param size Desired thumbnail size
   * @return Generated thumbnail pixbuf (caller takes ownership)
   */
  GdkPixbuf *generateSync(const std::string &wallpaperPath, Size size);

  /**
   * @brief Check if a thumbnail exists in cache
   */
  bool isCached(const std::string &wallpaperPath, Size size) const;

  /**
   * @brief Invalidate cache for a specific wallpaper
   */
  void invalidate(const std::string &wallpaperPath);

  /**
   * @brief Clear entire thumbnail cache
   */
  void clearCache();

  /**
   * @brief Get cache statistics
   */
  struct CacheStats {
    size_t cachedCount;
    size_t totalSizeBytes;
    size_t memoryUsageBytes;
  };
  CacheStats getStats() const;

  /**
   * @brief Set maximum cache size in MB
   */
  void setMaxCacheSize(size_t megabytes);

  /**
   * @brief Prune cache to stay within size limit (LRU eviction)
   */
  void pruneCache();

private:
  ThumbnailCache();
  ~ThumbnailCache();

  ThumbnailCache(const ThumbnailCache &) = delete;
  ThumbnailCache &operator=(const ThumbnailCache &) = delete;

  // Generate a cache key (hash) from wallpaper path
  std::string generateCacheKey(const std::string &wallpaperPath) const;

  // Get the cache file path for a wallpaper
  std::filesystem::path getCachePath(const std::string &wallpaperPath,
                                     Size size) const;

  // Generate thumbnail from image file
  GdkPixbuf *generateFromImage(const std::string &path, Size size);

  // Generate thumbnail from video file (first frame)
  GdkPixbuf *generateFromVideo(const std::string &path, Size size);

  // Load a thumbnail from cache file
  GdkPixbuf *loadFromCache(const std::filesystem::path &cachePath);

  // Save a pixbuf to cache file
  bool saveToCache(GdkPixbuf *pixbuf, const std::filesystem::path &cachePath);

  // Initialize cache directory
  void initCacheDir();

  // Cache directory path
  std::filesystem::path m_cacheDir;

  // In-memory LRU cache for recently accessed thumbnails
  struct CacheEntry {
    GdkPixbuf *pixbuf;
    std::chrono::steady_clock::time_point lastAccess;
  };
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, CacheEntry> m_memoryCache;
  size_t m_maxMemoryCacheEntries = 100; // Max thumbnails in memory
  size_t m_maxDiskCacheMB = 500;        // Max disk cache size in MB
};

} // namespace bwp::wallpaper
