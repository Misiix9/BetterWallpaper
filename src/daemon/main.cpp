#include "../core/ipc/IPCServiceFactory.hpp"
#include "../core/slideshow/SlideshowManager.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/utils/Constants.hpp"
#include "../core/wallpaper/WallpaperManager.hpp"
#ifndef _WIN32
#include <gtk/gtk.h>
#endif
#include <iostream>
#include <memory>

// DaemonApp abstractions
#ifdef _WIN32
#include <windows.h>
#else
#include <gtk/gtk.h>
#endif

class DaemonApp {
public:
  DaemonApp(int argc, char **argv) : m_argc(argc), m_argv(argv) {
#ifndef _WIN32
    m_app = gtk_application_new(bwp::constants::APP_ID_DAEMON, G_APPLICATION_DEFAULT_FLAGS);
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
      // Windows Main Loop
      LOG_INFO("Daemon starting up (Windows)...");
      setupServices(this);
      
      // Keep running
      LOG_INFO("Daemon running. Press Ctrl+C to stop.");
      Sleep(INFINITE); // TODO: Better event loop or service controller
      return 0;
#else
      return g_application_run(G_APPLICATION(m_app), m_argc, m_argv);
#endif
  }

private:
#ifndef _WIN32
  static void onActivate(GtkApplication *app, gpointer /*user_data*/) {
    g_application_hold(G_APPLICATION(app)); // Keep running without windows
    LOG_INFO("Daemon activated");
  }

  static void onStartup(GtkApplication *app, gpointer user_data) {
    auto *self = static_cast<DaemonApp *>(user_data);
    setupServices(self);
  }
#endif

  static void setupServices(DaemonApp* self) {
    LOG_INFO("Initializing Services...");

    // Initialize Core Services
    bwp::wallpaper::WallpaperManager::getInstance().initialize();

    // Initialize IPC
    self->m_ipcService = bwp::ipc::IPCServiceFactory::createService();

    // Wire up handlers
    self->m_ipcService->setSetWallpaperHandler(
        [](const std::string &path, const std::string &monitor) -> bool {
          LOG_INFO("IPC Command: SetWallpaper " + path + " on " + monitor);
          return bwp::wallpaper::WallpaperManager::getInstance().setWallpaper(
              monitor, path);
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
    });

    self->m_ipcService->setResumeHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Resume on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().resume(monitor);
    });

    self->m_ipcService->setStopHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Stop on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().stop(monitor);
    });
    
    // Mute/Volume handlers (TODO: Add to manager)
    self->m_ipcService->setSetMutedHandler([](const std::string &monitor, bool muted) {
        LOG_INFO("IPC Command: Mute " + std::string(muted ? "ON" : "OFF"));
        bwp::wallpaper::WallpaperManager::getInstance().setMuted(monitor, muted);
    });
    
    self->m_ipcService->setSetVolumeHandler([](const std::string &monitor, int vol) {
        LOG_INFO("IPC Command: Volume " + std::to_string(vol));
        bwp::wallpaper::WallpaperManager::getInstance().setVolume(monitor, vol);
    });

    if (!self->m_ipcService->initialize()) {
      LOG_ERROR("Failed to initialize IPC Service.");
      // On Windows just log, on Linux quit
#ifndef _WIN32
       // g_application_quit(...); // Hard without app pointer here easily.
       exit(1);
#endif
    }
    
    // Set default wallpaper if standard path exists?
    // bwp::wallpaper::WallpaperManager::getInstance().setWallpaper("primary", "C:\\Users\\Default\\image.jpg");

    LOG_INFO("Daemon Service Ready.");
  }

  int m_argc;
  char **m_argv;
#ifndef _WIN32
  GtkApplication *m_app;
#endif
  std::unique_ptr<bwp::ipc::IIPCService> m_ipcService;
};

int main(int argc, char **argv) {
  // Init logger
#ifdef _WIN32
     std::string logDir = std::string(getenv("APPDATA")) + "\\BetterWallpaper";
     CreateDirectory(logDir.c_str(), NULL);
     bwp::utils::Logger::init(logDir);
#else
     std::string logDir = std::string(getenv("HOME")) + "/.cache/betterwallpaper";
     bwp::utils::Logger::init(logDir);
#endif

  DaemonApp app(argc, argv);
  return app.run();
}
