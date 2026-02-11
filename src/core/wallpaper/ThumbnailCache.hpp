#pragma once
#include <chrono>
#include <filesystem>
#include <functional>
#ifdef _WIN32
typedef struct _GdkPixbuf GdkPixbuf;
#else
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#include <mutex>
#include <string>
#include <unordered_map>
namespace bwp::wallpaper {
class ThumbnailCache {
public:
  static ThumbnailCache &getInstance();
  enum class Size {
    Small = 128,   
    Medium = 256,  
    Large = 512    
  };
  GdkPixbuf *getSync(const std::string &wallpaperPath, Size size);
  using ThumbnailCallback = std::function<void(GdkPixbuf *pixbuf)>;
  void getAsync(const std::string &wallpaperPath, Size size,
                ThumbnailCallback callback);
  GdkPixbuf *generateSync(const std::string &wallpaperPath, Size size);
  bool isCached(const std::string &wallpaperPath, Size size) const;
  void invalidate(const std::string &wallpaperPath);
  void clearCache();
  struct CacheStats {
    size_t cachedCount;
    size_t totalSizeBytes;
    size_t memoryUsageBytes;
  };
  CacheStats getStats() const;
  void setMaxCacheSize(size_t megabytes);
  void pruneCache();
  std::string computeBlurhash(const std::string &wallpaperPath, Size size);
private:
  ThumbnailCache();
  ~ThumbnailCache();
  ThumbnailCache(const ThumbnailCache &) = delete;
  ThumbnailCache &operator=(const ThumbnailCache &) = delete;
  std::string generateCacheKey(const std::string &wallpaperPath) const;
  std::filesystem::path getCachePath(const std::string &wallpaperPath,
                                     Size size) const;
  GdkPixbuf *generateFromImage(const std::string &path, Size size);
  GdkPixbuf *generateFromVideo(const std::string &path, Size size);
  GdkPixbuf *loadFromCache(const std::filesystem::path &cachePath);
  bool saveToCache(GdkPixbuf *pixbuf, const std::filesystem::path &cachePath);
  void initCacheDir();
  std::filesystem::path m_cacheDir;
  struct CacheEntry {
    GdkPixbuf *pixbuf;
    std::chrono::steady_clock::time_point lastAccess;
  };
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, CacheEntry> m_memoryCache;
  size_t m_maxMemoryCacheEntries = 100;  
  size_t m_maxDiskCacheMB = 500;         
};
}  
