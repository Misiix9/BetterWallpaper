#include "SlideshowManager.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include "../wallpaper/WallpaperLibrary.hpp"
#include <algorithm>
#include <random>
#ifdef _WIN32
typedef int gboolean;
typedef void* gpointer;
typedef unsigned int guint;
#define G_SOURCE_CONTINUE true
#define G_SOURCE_REMOVE false
inline guint g_timeout_add_seconds(guint interval, int (*func)(void*), void* data) {
    return 1;  
}
inline void g_source_remove(guint tag) {}
#else
#include <glib.h>
#endif
namespace bwp::core {
SlideshowManager &SlideshowManager::getInstance() {
  static SlideshowManager instance;
  return instance;
}
SlideshowManager::~SlideshowManager() { stop(); }
void SlideshowManager::start(const std::vector<std::string> &wallpaperIds,
                             int intervalSeconds) {
  stop();  
  m_playlist = wallpaperIds;
  m_intervalSeconds = intervalSeconds;
  m_currentIndex = 0;
  if (m_playlist.empty()) {
    LOG_WARN("Cannot start slideshow with empty playlist");
    return;
  }
  if (m_shuffle) {
    shufflePlaylist();
  }
  m_running = true;
  m_paused = false;
  applyCurrentWallpaper();
  m_timerId = g_timeout_add_seconds(
      m_intervalSeconds,
      [](gpointer data) -> gboolean {
        auto *self = static_cast<SlideshowManager *>(data);
        self->tick();
        return G_SOURCE_CONTINUE;
      },
      this);
  LOG_INFO("Slideshow started with " + std::to_string(m_playlist.size()) +
           " wallpapers, interval: " + std::to_string(m_intervalSeconds) + "s");
  if (!m_loading) {
    saveToConfig();
  }
}
void SlideshowManager::startFromFolder(const std::string &folderPath,
                                       int intervalSeconds) {
  auto &library = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto wallpapers = library.getAllWallpapers();
  std::vector<std::string> ids;
  for (const auto &wp : wallpapers) {
    if (wp.path.find(folderPath) != std::string::npos) {
      ids.push_back(wp.id);
    }
  }
  start(ids, intervalSeconds);
}
void SlideshowManager::startFromTag(const std::string &tag,
                                    int intervalSeconds) {
  auto &library = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto wallpapers = library.getAllWallpapers();
  std::vector<std::string> ids;
  for (const auto &wp : wallpapers) {
    for (const auto &t : wp.tags) {
      if (t == tag) {
        ids.push_back(wp.id);
        break;
      }
    }
  }
  start(ids, intervalSeconds);
}
void SlideshowManager::stop() {
  if (m_timerId > 0) {
    g_source_remove(m_timerId);
    m_timerId = 0;
  }
  m_running = false;
  m_paused = false;
  m_playlist.clear();
  m_currentIndex = 0;
  LOG_INFO("Slideshow stopped");
  saveToConfig();
}
void SlideshowManager::pause() {
  if (!m_running || m_paused)
    return;
  if (m_timerId > 0) {
    g_source_remove(m_timerId);
    m_timerId = 0;
  }
  m_paused = true;
  LOG_INFO("Slideshow paused");
}
void SlideshowManager::resume() {
  if (!m_running || !m_paused)
    return;
  m_paused = false;
  m_timerId = g_timeout_add_seconds(
      m_intervalSeconds,
      [](gpointer data) -> gboolean {
        auto *self = static_cast<SlideshowManager *>(data);
        self->tick();
        return G_SOURCE_CONTINUE;
      },
      this);
  LOG_INFO("Slideshow resumed");
}
void SlideshowManager::next() {
  if (!m_running || m_playlist.empty())
    return;
  m_currentIndex = (m_currentIndex + 1) % static_cast<int>(m_playlist.size());
  applyCurrentWallpaper();
  if (m_timerId > 0) {
    g_source_remove(m_timerId);
  }
  if (!m_paused) {
    m_timerId = g_timeout_add_seconds(
        m_intervalSeconds,
        [](gpointer data) -> gboolean {
          auto *self = static_cast<SlideshowManager *>(data);
          self->tick();
          return G_SOURCE_CONTINUE;
        },
        this);
  }
}
void SlideshowManager::previous() {
  if (!m_running || m_playlist.empty())
    return;
  m_currentIndex--;
  if (m_currentIndex < 0) {
    m_currentIndex = static_cast<int>(m_playlist.size()) - 1;
  }
  applyCurrentWallpaper();
  if (m_timerId > 0) {
    g_source_remove(m_timerId);
  }
  if (!m_paused) {
    m_timerId = g_timeout_add_seconds(
        m_intervalSeconds,
        [](gpointer data) -> gboolean {
          auto *self = static_cast<SlideshowManager *>(data);
          self->tick();
          return G_SOURCE_CONTINUE;
        },
        this);
  }
}
std::string SlideshowManager::getCurrentWallpaperId() const {
  if (m_playlist.empty() || m_currentIndex < 0 ||
      m_currentIndex >= static_cast<int>(m_playlist.size())) {
    return "";
  }
  return m_playlist[m_currentIndex];
}
void SlideshowManager::setShuffle(bool shuffle) {
  m_shuffle = shuffle;
  if (m_shuffle && m_running) {
    shufflePlaylist();
    m_currentIndex = 0;
  }
  saveToConfig();
}
void SlideshowManager::tick() {
  if (m_paused)
    return;
  m_currentIndex = (m_currentIndex + 1) % static_cast<int>(m_playlist.size());
  applyCurrentWallpaper();
}
void SlideshowManager::applyCurrentWallpaper() {
  std::string id = getCurrentWallpaperId();
  if (id.empty())
    return;
  LOG_DEBUG("Slideshow: applying wallpaper " +
            std::to_string(m_currentIndex + 1) + "/" +
            std::to_string(m_playlist.size()));
  if (m_changeCallback) {
    m_changeCallback(id);
  }
}
void SlideshowManager::shufflePlaylist() {
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(m_playlist.begin(), m_playlist.end(), g);
}
void SlideshowManager::loadFromConfig() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  m_shuffle = conf.get<bool>("slideshow.shuffle");
  m_intervalSeconds = conf.get<int>("slideshow.interval");
  if (m_intervalSeconds < 10) {
    m_intervalSeconds = 300;  
  }
  bool wasRunning = conf.get<bool>("slideshow.running");
  if (wasRunning) {
    auto playlist = conf.get<std::vector<std::string>>("slideshow.playlist");
    if (!playlist.empty()) {
      m_loading = true;   
      start(playlist, m_intervalSeconds);
      m_loading = false;
      m_currentIndex = conf.get<int>("slideshow.current_index");
      if (m_currentIndex >= static_cast<int>(m_playlist.size())) {
        m_currentIndex = 0;
      }
    }
  }
}
void SlideshowManager::saveToConfig() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  conf.set("slideshow.running", m_running);
  conf.set("slideshow.shuffle", m_shuffle);
  conf.set("slideshow.interval", m_intervalSeconds);
  conf.set("slideshow.playlist", m_playlist);
  conf.set("slideshow.current_index", m_currentIndex);
}
}  
