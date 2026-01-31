#include "../core/ipc/DBusService.hpp"
#include "../core/slideshow/SlideshowManager.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/wallpaper/WallpaperManager.hpp"
#include <gtk/gtk.h>
#include <iostream>
#include <memory>

// Define a simple application class to manage lifecycle
class DaemonApp {
public:
  DaemonApp(int argc, char **argv) : m_argc(argc), m_argv(argv) {
    m_app = gtk_application_new("com.github.betterwallpaper.daemon",
                                G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(m_app, "activate", G_CALLBACK(onActivate), this);
    g_signal_connect(m_app, "startup", G_CALLBACK(onStartup), this);
  }

  ~DaemonApp() { g_object_unref(m_app); }

  int run() { return g_application_run(G_APPLICATION(m_app), m_argc, m_argv); }

private:
  static void onActivate(GtkApplication *app, gpointer /*user_data*/) {
    g_application_hold(G_APPLICATION(app)); // Keep running without windows
    LOG_INFO("Daemon activated");
  }

  static void onStartup(GtkApplication *app, gpointer user_data) {
    auto *self = static_cast<DaemonApp *>(user_data);
    LOG_INFO("Daemon starting up...");

    // Initialize Core Services
    bwp::wallpaper::WallpaperManager::getInstance().initialize();

    // Initialize IPC
    self->m_dbusService = std::make_unique<bwp::ipc::DBusService>();

    // Wire up handlers
    self->m_dbusService->setSetWallpaperHandler(
        [](const std::string &path, const std::string &monitor) -> bool {
          LOG_INFO("IPC Command: SetWallpaper " + path + " on " + monitor);
          return bwp::wallpaper::WallpaperManager::getInstance().setWallpaper(
              monitor, path);
        });

    self->m_dbusService->setNextHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Next on " + monitor);
      bwp::core::SlideshowManager::getInstance().next();
    });

    self->m_dbusService->setPreviousHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Previous on " + monitor);
      bwp::core::SlideshowManager::getInstance().previous();
    });

    self->m_dbusService->setPauseHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Pause on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().pause(monitor);
    });

    self->m_dbusService->setResumeHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Resume on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().resume(monitor);
    });

    self->m_dbusService->setStopHandler([](const std::string &monitor) {
      LOG_INFO("IPC Command: Stop on " + monitor);
      bwp::wallpaper::WallpaperManager::getInstance().stop(monitor);
    });

    if (!self->m_dbusService->initialize()) {
      LOG_ERROR(
          "Failed to initialize DBus Service. Is another instance running?");
      g_application_quit(G_APPLICATION(app));
    }

    LOG_INFO("Daemon ready.");
  }

  int m_argc;
  char **m_argv;
  GtkApplication *m_app;
  std::unique_ptr<bwp::ipc::DBusService> m_dbusService;
};

int main(int argc, char **argv) {
  DaemonApp app(argc, argv);
  return app.run();
}
