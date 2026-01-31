#pragma once
#include <functional>
#include <glib.h>
#include <string>
#include <vector>

namespace bwp::core {

/**
 * Manages automatic wallpaper cycling (slideshow) functionality.
 * Can cycle through wallpapers in a folder or playlist at specified intervals.
 */
class SlideshowManager {
public:
  using WallpaperChangeCallback =
      std::function<void(const std::string &wallpaperId)>;

  static SlideshowManager &getInstance();

  // Start a slideshow
  void start(const std::vector<std::string> &wallpaperIds, int intervalSeconds);
  void startFromFolder(const std::string &folderPath, int intervalSeconds);
  void startFromTag(const std::string &tag, int intervalSeconds);

  // Control
  void stop();
  void pause();
  void resume();
  void next();
  void previous();

  // State
  bool isRunning() const { return m_running && !m_paused; }
  bool isPaused() const { return m_paused; }
  int getIntervalSeconds() const { return m_intervalSeconds; }
  int getCurrentIndex() const { return m_currentIndex; }
  size_t getPlaylistSize() const { return m_playlist.size(); }
  std::string getCurrentWallpaperId() const;

  // Shuffle
  void setShuffle(bool shuffle);
  bool isShuffle() const { return m_shuffle; }

  // Callback when wallpaper changes
  void setChangeCallback(WallpaperChangeCallback callback) {
    m_changeCallback = callback;
  }

  // Load/save state
  void loadFromConfig();
  void saveToConfig();

private:
  SlideshowManager() = default;
  ~SlideshowManager();

  SlideshowManager(const SlideshowManager &) = delete;
  SlideshowManager &operator=(const SlideshowManager &) = delete;

  void tick();
  void applyCurrentWallpaper();
  void shufflePlaylist();

  std::vector<std::string> m_playlist;
  std::vector<std::string> m_shuffledIndices;

  int m_currentIndex = 0;
  int m_intervalSeconds = 300; // 5 minutes default
  bool m_running = false;
  bool m_paused = false;
  bool m_shuffle = false;

  guint m_timerId = 0;
  WallpaperChangeCallback m_changeCallback;
};

} // namespace bwp::core
