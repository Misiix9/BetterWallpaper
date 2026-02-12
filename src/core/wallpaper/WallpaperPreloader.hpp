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
class WallpaperPreloader {
public:
  static WallpaperPreloader &getInstance();
  enum class PreloadState { None, Loading, Ready, Failed };
  using PreloadCallback =
      std::function<void(const std::string &path, PreloadState state)>;
  void preload(const std::string &path, PreloadCallback callback = nullptr);
  PreloadState getState(const std::string &path) const;
  bool isReady(const std::string &path) const;
  std::shared_ptr<WallpaperRenderer>
  getPreloadedRenderer(const std::string &path);
  void clearPreload(const std::string &path = "");
  void cancelPreload(const std::string &path);
  void setMaxPreloads(size_t max);

private:
  WallpaperPreloader();
  ~WallpaperPreloader();
  WallpaperPreloader(const WallpaperPreloader &) = delete;
  WallpaperPreloader &operator=(const WallpaperPreloader &) = delete;
  struct PreloadEntry {
    std::string path;
    PreloadState state = PreloadState::None;
    std::shared_ptr<WallpaperRenderer> renderer;
    std::thread preloadThread;
    std::atomic<bool> cancelled{false};
    PreloadCallback callback;
    int64_t preloadedAtMs = 0;

    ~PreloadEntry() {
      if (preloadThread.joinable()) {
        preloadThread.detach();
      }
    }
  };
  void doPreload(const std::string &path);
  void evictOldPreloads();
  WallpaperType detectType(const std::string &path) const;
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, std::shared_ptr<PreloadEntry>> m_preloads;
  size_t m_maxPreloads = 3;
};
} // namespace bwp::wallpaper
