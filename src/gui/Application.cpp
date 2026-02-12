#include "Application.hpp"
#include "../core/config/ConfigManager.hpp"
#include "../core/monitor/MonitorManager.hpp"
#include "../core/steam/SteamService.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/wallpaper/WallpaperManager.hpp"
#include "MainWindow.hpp"
#include "dialogs/FluidSetupWizard.hpp"
#include <iostream>
#include <wayland-client.h>
namespace bwp::gui {
static Application *s_instance = nullptr;
GtkCssProvider *Application::s_cssProvider = nullptr;
GFileMonitor *Application::s_cssMonitor = nullptr;
std::string Application::s_currentCssPath;
Application *Application::create() {
  if (!s_instance) {
    s_instance = new Application();
  }
  return s_instance;
}
Application *Application::getInstance() { return s_instance; }
Application::Application() {
  m_app = adw_application_new("com.github.betterwallpaper",
                              G_APPLICATION_HANDLES_COMMAND_LINE);
  g_signal_connect(m_app, "activate", G_CALLBACK(onActivate), this);
  g_signal_connect(m_app, "startup", G_CALLBACK(onStartup), this);
  g_signal_connect(m_app, "command-line", G_CALLBACK(onCommandLine), this);
}
Application::~Application() {
  bwp::wallpaper::WallpaperManager::getInstance().shutdown();
  if (s_cssMonitor) {
    g_object_unref(s_cssMonitor);
    s_cssMonitor = nullptr;
  }
  if (s_cssProvider) {
    g_object_unref(s_cssProvider);
    s_cssProvider = nullptr;
  }
  g_object_unref(m_app);
}
int Application::run(int argc, char **argv) {
  return g_application_run(G_APPLICATION(m_app), argc, argv);
}
int Application::onCommandLine(GApplication *app,
                               GApplicationCommandLine *cmdline,
                               gpointer user_data) {
  // Ensure the application is activated (window created/shown)
  g_application_activate(app);

  // Application *self = static_cast<Application *>(user_data);
  // Get arguments
  int argc;
  char **argv = g_application_command_line_get_arguments(cmdline, &argc);

  // Find the MainWindow if it exists
  GtkWindow *activeWindow =
      gtk_application_get_active_window(GTK_APPLICATION(app));
  MainWindow *mainWindow = nullptr;
  if (activeWindow) {
    mainWindow = static_cast<MainWindow *>(
        g_object_get_data(G_OBJECT(activeWindow), "main-window"));
  }

  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--settings") {
      bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                              "CLI: Opening Settings");
      if (mainWindow) {
        mainWindow->navigateTo("settings");
        mainWindow->show();
      }
    } else if (arg == "--show" || arg == "--library") {
      bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                              "CLI: Showing Library");
      if (mainWindow) {
        mainWindow->navigateTo("library");
        mainWindow->show();
      }
    }
  }
  g_strfreev(argv);
  return 0;
}
void Application::onActivate(GApplication *app, gpointer) {
  bool showWizard = bwp::config::ConfigManager::getInstance().get<bool>(
      "general.first_run", true);
  if (showWizard) {
    bwp::utils::Logger::log(
        bwp::utils::LogLevel::INFO,
        "Launching Fluid Setup Wizard (main app hidden until complete)");
    auto *wizard = new FluidSetupWizard(nullptr);
    wizard->setOnComplete([app, wizard]() {
      bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                              "Wizard complete - launching main application");
      auto *window = new MainWindow(ADW_APPLICATION(app));
      window->show();
      delete wizard;
    });
    wizard->show();
  } else {
    bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                            "Activating application, creating MainWindow...");
    auto *window = new MainWindow(ADW_APPLICATION(app));
    window->show();
  }
}
void Application::onStartup(GApplication *, gpointer) {
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO, "Application startup");
  if (!bwp::monitor::MonitorManager::getInstance().initialize()) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::ERR,
                            "Failed to initialize MonitorManager");
  }
  // Load saved Steam API key from config into memory
  bwp::steam::SteamService::getInstance().initialize();
  loadStylesheet();
  setupCssHotReload();
  ensureBackgroundServices();
}
std::string Application::findStylesheetPath() {
  std::vector<std::string> searchPaths;
  const char *xdgDataHome = std::getenv("XDG_DATA_HOME");
  if (xdgDataHome && std::strlen(xdgDataHome) > 0) {
    searchPaths.push_back(std::string(xdgDataHome) +
                          "/betterwallpaper/ui/style.css");
  } else {
    const char *home = std::getenv("HOME");
    if (home) {
      searchPaths.push_back(std::string(home) +
                            "/.local/share/betterwallpaper/ui/style.css");
    }
  }
  const char *xdgDataDirs = std::getenv("XDG_DATA_DIRS");
  std::string dataDirs =
      xdgDataDirs ? xdgDataDirs : "/usr/local/share:/usr/share";
  size_t pos = 0;
  while ((pos = dataDirs.find(':')) != std::string::npos) {
    std::string dir = dataDirs.substr(0, pos);
    if (!dir.empty())
      searchPaths.push_back(dir + "/betterwallpaper/ui/style.css");
    dataDirs.erase(0, pos + 1);
  }
  if (!dataDirs.empty()) {
    searchPaths.push_back(dataDirs + "/betterwallpaper/ui/style.css");
  }
  std::filesystem::path exePath;
  try {
    exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
    searchPaths.push_back((exePath / "../data/ui/style.css").string());
    searchPaths.push_back((exePath / "../../data/ui/style.css").string());
    searchPaths.push_back((exePath / "../../../data/ui/style.css").string());
  } catch (const std::exception &e) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARN,
                            "Failed to get executable path: " +
                                std::string(e.what()));
  }
  searchPaths.push_back("data/ui/style.css");
  searchPaths.push_back("../data/ui/style.css");
  for (const auto &path : searchPaths) {
    try {
      if (std::filesystem::exists(path)) {
        std::filesystem::path canonicalPath = std::filesystem::canonical(path);
        bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                                "Found stylesheet at: " +
                                    canonicalPath.string());
        return canonicalPath.string();
      }
    } catch (const std::exception &e) {
      (void)e;
    }
  }
  return "";
}
void Application::loadStylesheet() {
  std::string cssPath = findStylesheetPath();
  if (cssPath.empty()) {
    bwp::utils::Logger::log(
        bwp::utils::LogLevel::ERR,
        "Could not find style.css - UI may look incorrect. "
        "Searched in XDG data directories and development paths.");
    return;
  }
  s_currentCssPath = cssPath;
  if (!s_cssProvider) {
    s_cssProvider = gtk_css_provider_new();
  }
  gtk_css_provider_load_from_path(s_cssProvider, cssPath.c_str());
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::ERR,
                            "Could not get default display for CSS loading");
    return;
  }
  gtk_style_context_add_provider_for_display(display,
                                             GTK_STYLE_PROVIDER(s_cssProvider),
                                             GTK_STYLE_PROVIDER_PRIORITY_USER);
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "Successfully loaded stylesheet from: " + cssPath);
  auto *mmDisplay = bwp::monitor::MonitorManager::getInstance().getDisplay();
  if (mmDisplay) {
    int fd = wl_display_get_fd(mmDisplay);
    GIOChannel *channel = g_io_channel_unix_new(fd);
    g_io_add_watch(
        channel, G_IO_IN,
        +[](GIOChannel *, GIOCondition, gpointer) -> gboolean {
          bwp::monitor::MonitorManager::getInstance().update();
          return TRUE;
        },
        nullptr);
    g_io_channel_unref(channel);
  } else {
    bwp::utils::Logger::log(
        bwp::utils::LogLevel::WARN,
        "MonitorManager has no display, hotplug detection disabled");
  }
}
void Application::setupCssHotReload() {
  if (s_currentCssPath.empty()) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARN,
                            "Cannot setup CSS hot-reload: no CSS file loaded");
    return;
  }
  GFile *cssFile = g_file_new_for_path(s_currentCssPath.c_str());
  GError *error = nullptr;
  s_cssMonitor =
      g_file_monitor_file(cssFile, G_FILE_MONITOR_NONE, nullptr, &error);
  g_object_unref(cssFile);
  if (error) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARN,
                            "Failed to setup CSS hot-reload: " +
                                std::string(error->message));
    g_error_free(error);
    return;
  }
  if (s_cssMonitor) {
    g_signal_connect(s_cssMonitor, "changed", G_CALLBACK(onCssFileChanged),
                     nullptr);
    bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                            "CSS hot-reload enabled for: " + s_currentCssPath);
  }
}
void Application::onCssFileChanged(GFileMonitor *monitor, GFile *file,
                                   GFile *other_file,
                                   GFileMonitorEvent event_type,
                                   gpointer user_data) {
  (void)monitor;
  (void)file;
  (void)other_file;
  (void)user_data;
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED ||
      event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                            "CSS file changed, reloading stylesheet...");
    reloadStylesheet();
  }
}
void Application::reloadStylesheet() {
  if (s_currentCssPath.empty() || !s_cssProvider) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARN,
                            "Cannot reload stylesheet: no CSS file loaded");
    return;
  }
  gtk_css_provider_load_from_path(s_cssProvider, s_currentCssPath.c_str());
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "Stylesheet reloaded successfully");
}
void Application::ensureBackgroundServices() {
  spawnProcess("betterwallpaper-daemon", "../daemon/betterwallpaper-daemon");
  spawnProcess("betterwallpaper-tray", "../tray/betterwallpaper-tray");
}
bool Application::spawnProcess(const std::string &name,
                               const std::string &relativePath) {
  std::filesystem::path exePath;
  try {
    exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
    std::filesystem::path fullPath = exePath / relativePath;
    if (std::filesystem::exists(fullPath)) {
      bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                              "Spawning: " + fullPath.string());
      char *argv[] = {g_strdup(fullPath.c_str()), nullptr};
      GError *err = nullptr;
      g_spawn_async(nullptr, argv, nullptr, G_SPAWN_SEARCH_PATH, nullptr,
                    nullptr, nullptr, &err);
      g_free(argv[0]);
      if (err) {
        bwp::utils::Logger::log(bwp::utils::LogLevel::ERR, "Failed to spawn " +
                                                               name + ": " +
                                                               err->message);
        g_error_free(err);
        return false;
      }
      return true;
    }
    bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                            "Spawning system " + name);
    char *argv[] = {g_strdup(name.c_str()), nullptr};
    GError *err = nullptr;
    g_spawn_async(nullptr, argv, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr,
                  nullptr, &err);
    g_free(argv[0]);
    if (err) {
      g_error_free(err);
    }
  } catch (...) {
    return false;
  }
  return true;
}
} // namespace bwp::gui
