#pragma once

#include "WallpaperInfo.hpp"
#include "WallpaperRenderer.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace bwp::wallpaper {

/**
 * WallpaperPreloader handles preloading wallpapers in the background
 * when they are selected (but not yet set) to make the "Set as Wallpaper"
 * action instant.
 *
 * Preloading includes:
 * - For static images: Load into memory
 * - For videos: Initialize MPV decoder with first frame ready
 * - For WE scenes: Validate assets exist, pre-warm (if possible)
 */
class WallpaperPreloader {
public:
  static WallpaperPreloader &getInstance();

  // Preload state
  enum class PreloadState { None, Loading, Ready, Failed };

  // Callback when preload completes
  using PreloadCallback = std::function<void(const std::string &path,
                                             PreloadState state)>;

  /**
   * Start preloading a wallpaper in the background.
   * Call this when user selects a wallpaper card.
   *
   * @param path The wallpaper path to preload
   * @param callback Optional callback when preload completes
   */
  void preload(const std::string &path, PreloadCallback callback = nullptr);

  /**
   * Check if a wallpaper is preloaded and ready.
   *
   * @param path The wallpaper path to check
   * @return PreloadState indicating if ready, loading, or not preloaded
   */
  PreloadState getState(const std::string &path) const;

  /**
   * Check if a wallpaper is ready to be set instantly.
   *
   * @param path The wallpaper path
   * @return true if preloaded and ready
   */
  bool isReady(const std::string &path) const;

  /**
   * Get the preloaded renderer for a wallpaper (if available).
   * This allows instant activation without re-creating the renderer.
   *
   * @param path The wallpaper path
   * @return Shared pointer to renderer, or nullptr if not preloaded
   */
  std::shared_ptr<WallpaperRenderer> getPreloadedRenderer(const std::string &path);

  /**
   * Clear preload cache for a specific path or all paths.
   *
   * @param path Optional path to clear. If empty, clears all.
   */
  void clearPreload(const std::string &path = "");

  /**
   * Cancel ongoing preload for a path.
   */
  void cancelPreload(const std::string &path);

  /**
   * Set maximum number of wallpapers to keep preloaded.
   * Older preloads are evicted when limit is reached.
   */
  void setMaxPreloads(size_t max);

private:
  WallpaperPreloader();
  ~WallpaperPreloader();

  WallpaperPreloader(const WallpaperPreloader &) = delete;
  WallpaperPreloader &operator=(const WallpaperPreloader &) = delete;

  // Internal preload state
  struct PreloadEntry {
    std::string path;
    PreloadState state = PreloadState::None;
    std::shared_ptr<WallpaperRenderer> renderer;
    std::thread preloadThread;
    std::atomic<bool> cancelled{false};
    PreloadCallback callback;
    int64_t preloadedAtMs = 0;
  };

  // Actual preload work
  void doPreload(const std::string &path);

  // Evict old preloads if over limit
  void evictOldPreloads();

  // Determine wallpaper type from path
  WallpaperType detectType(const std::string &path) const;

  mutable std::mutex m_mutex;
  std::unordered_map<std::string, std::shared_ptr<PreloadEntry>> m_preloads;
  size_t m_maxPreloads = 3;
};

} // namespace bwp::wallpaper
