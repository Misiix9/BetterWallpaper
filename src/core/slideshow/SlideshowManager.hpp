#pragma once
#include <functional>
#ifdef _WIN32
typedef unsigned int guint;
#else
#include <glib.h>
#endif
#include <string>
#include <vector>
namespace bwp::core {
class SlideshowManager {
public:
  using WallpaperChangeCallback =
      std::function<void(const std::string &wallpaperId)>;
  static SlideshowManager &getInstance();
  void start(const std::vector<std::string> &wallpaperIds, int intervalSeconds);
  void startFromFolder(const std::string &folderPath, int intervalSeconds);
  void startFromTag(const std::string &tag, int intervalSeconds);
  void stop();
  void pause();
  void resume();
  void next();
  void previous();
  bool isRunning() const { return m_running && !m_paused; }
  bool isPaused() const { return m_paused; }
  int getIntervalSeconds() const { return m_intervalSeconds; }
  int getCurrentIndex() const { return m_currentIndex; }
  size_t getPlaylistSize() const { return m_playlist.size(); }
  std::string getCurrentWallpaperId() const;
  void setShuffle(bool shuffle);
  bool isShuffle() const { return m_shuffle; }
  void setChangeCallback(WallpaperChangeCallback callback) {
    m_changeCallback = callback;
  }
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
  int m_intervalSeconds = 300;  
  bool m_running = false;
  bool m_paused = false;
  bool m_shuffle = false;
  bool m_loading = false;   
  guint m_timerId = 0;
  WallpaperChangeCallback m_changeCallback;
};
}  
