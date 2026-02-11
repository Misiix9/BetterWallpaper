#include "../core/ipc/IPCServiceFactory.hpp"
#include "../core/monitor/MonitorManager.hpp"
#include "../core/slideshow/SlideshowManager.hpp"
#include "../core/utils/Constants.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/wallpaper/WallpaperManager.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <gio/gio.h>
#include <glib-unix.h>
#endif
class DaemonApp {
public:
  DaemonApp(int argc, char **argv) : m_argc(argc), m_argv(argv) {
#ifndef _WIN32
    m_app = g_application_new(bwp::constants::APP_ID_DAEMON,
                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(m_app, "activate", G_CALLBACK(onActivate), this);
    g_signal_connect(m_app, "startup", G_CALLBACK(onStartup), this);
#endif
  }
  ~DaemonApp() {
#ifndef _WIN32
    g_object_unref(m_app);
#endif
  }
  int run() {
#ifdef _WIN32
    LOG_INFO("Daemon starting up (Windows)...");
    if (!setupServices(this)) return 1;
    LOG_INFO("Daemon running. Press Ctrl+C to stop.");
    Sleep(INFINITE);  
    return 0;
#else
    return g_application_run(m_app, m_argc, m_argv);
#endif
  }
private:
#ifndef _WIN32
  static void onActivate(GApplication *app, gpointer) {
    g_application_hold(app);

    // Install signal handlers for graceful shutdown
    g_unix_signal_add(SIGTERM, onSignal, app);
    g_unix_signal_add(SIGINT, onSignal, app);

    LOG_INFO("Daemon activated");
  }
  static void onStartup(GApplication *, gpointer user_data) {
    auto *self = static_cast<DaemonApp *>(user_data);
    if (!setupServices(self)) {
      LOG_ERROR("Service setup failed, requesting shutdown.");
      g_application_quit(self->m_app);
    }
  }
  static gboolean onSignal(gpointer user_data) {
    LOG_INFO("Received shutdown signal, saving state...");
    bwp::wallpaper::WallpaperManager::getInstance().saveState();
    auto *app = static_cast<GApplication *>(user_data);
    g_application_quit(app);
    return G_SOURCE_REMOVE;
  }
#endif
  static bool setupServices(DaemonApp *self) {
    LOG_INFO("Initializing Services...");
    bwp::wallpaper::WallpaperManager::getInstance().initialize();
    LOG_INFO("Restoring wallpaper state from previous session...");
    bwp::wallpaper::WallpaperManager::getInstance().loadState();

    LOG_INFO("Restoring slideshow state...");
    bwp::core::SlideshowManager::getInstance().loadFromConfig();

    self->m_ipcService = bwp::ipc::IPCServiceFactory::createService();
    self->m_ipcService->setSetWallpaperHandler(
        [](const std::string &path, const std::string &monitor) -> bool {
          LOG_INFO("IPC Command: SetWallpaper " + path + " on " + monitor);
          return bwp::wallpaper::WallpaperManager::getInstance().setWallpaper(
              monitor, path);
        });
    self->m_ipcService->setGetWallpaperHandler(
        [](const std::string &monitor) -> std::string {
          LOG_INFO("IPC Command: GetWallpaper on " + monitor);
          return bwp::wallpaper::WallpaperManager::getInstance()
              .getCurrentWallpaper(monitor);
        });
    self->m_ipcService->setNextHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Next on " + monitor);
      bwp::core::SlideshowManager::getInstance().next();
    });
    self->m_ipcService->setPreviousHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Previous on " + monitor);
      bwp::core::SlideshowManager::getInstance().previous();
    });
    self->m_ipcService->setPauseHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Pause on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().pause(monitor);
      bwp::core::SlideshowManager::getInstance().pause();
    });
    self->m_ipcService->setResumeHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Resume on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().resume(monitor);
      bwp::core::SlideshowManager::getInstance().resume();
    });
    self->m_ipcService->setStopHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Stop on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().stop(monitor);
    });
    self->m_ipcService->setSetMutedHandler([](const std::string &monitor,
                                              bool muted) {
      LOG_INFO("IPC Command: Mute " + std::string(muted ? "ON" : "OFF"));
      bwp::wallpaper::WallpaperManager::getInstance().setMuted(monitor, muted);
    });
    self->m_ipcService->setSetVolumeHandler([](const std::string &monitor,
                                               int vol) {
      LOG_INFO("IPC Command: Volume " + std::to_string(vol));
      bwp::wallpaper::WallpaperManager::getInstance().setVolume(monitor, vol);
    });
    self->m_ipcService->setGetStatusHandler([]() -> std::string {
      LOG_INFO("IPC Command: GetStatus");
      nlohmann::json j;
      auto &wm = bwp::wallpaper::WallpaperManager::getInstance();
      auto &sm = bwp::core::SlideshowManager::getInstance();
      auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();
      j["daemon_version"] = BWP_VERSION;
      j["paused"] = wm.isPaused();
      j["muted"] = wm.isMuted();
      j["volume"] = wm.getVolume();
      j["slideshow_running"] = sm.isRunning();
      j["slideshow_paused"] = sm.isPaused();
      j["slideshow_index"] = sm.getCurrentIndex();
      j["slideshow_count"] = sm.getPlaylistSize();
      j["slideshow_interval"] = sm.getIntervalSeconds();
      j["slideshow_shuffle"] = sm.isShuffle();
      nlohmann::json monArr = nlohmann::json::array();
      for (const auto &mon : monitors) {
        nlohmann::json m;
        m["name"] = mon.name;
        m["wallpaper"] = wm.getCurrentWallpaper(mon.name);
        monArr.push_back(m);
      }
      j["monitors"] = monArr;
      return j.dump();
    });
    self->m_ipcService->setGetMonitorsHandler([]() -> std::string {
      LOG_INFO("IPC Command: GetMonitors");
      auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();
      nlohmann::json arr = nlohmann::json::array();
      for (const auto &mon : monitors) {
        arr.push_back(mon.name);
      }
      return arr.dump();
    });
    if (!self->m_ipcService->initialize()) {
      LOG_ERROR("Failed to initialize IPC Service.");
      return false;
    }
    LOG_INFO("Daemon Service Ready.");
    return true;
  }
  int m_argc;
  char **m_argv;
#ifndef _WIN32
  GApplication *m_app;
#endif
  std::unique_ptr<bwp::ipc::IIPCService> m_ipcService;
};
int main(int argc, char **argv) {
#ifdef _WIN32
  const char *appdata = std::getenv("APPDATA");
  if (!appdata) {
    std::cerr << "APPDATA environment variable not set" << std::endl;
    return 1;
  }
  std::string logDir = std::string(appdata) + "\\BetterWallpaper";
  CreateDirectory(logDir.c_str(), NULL);
  bwp::utils::Logger::init(logDir);
#else
  const char *home = std::getenv("HOME");
  if (!home) {
    std::cerr << "HOME environment variable not set" << std::endl;
    return 1;
  }
  std::string logDir = std::string(home) + "/.cache/betterwallpaper";
  bwp::utils::Logger::init(logDir);
#endif
  DaemonApp app(argc, argv);
  return app.run();
}
