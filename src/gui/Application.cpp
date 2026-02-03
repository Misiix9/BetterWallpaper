#include "Application.hpp"
#include "../core/config/ConfigManager.hpp"
#include "../core/monitor/MonitorManager.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/wallpaper/WallpaperManager.hpp"
#include "MainWindow.hpp"
#include "dialogs/SetupDialog.hpp"
#include <iostream>
#include <wayland-client.h>

namespace bwp::gui {

static Application *s_instance = nullptr;

// Static member definitions for CSS hot-reload
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
                              G_APPLICATION_NON_UNIQUE);
  g_signal_connect(m_app, "activate", G_CALLBACK(onActivate), this);
  g_signal_connect(m_app, "startup", G_CALLBACK(onStartup), this);
}

Application::~Application() {
  // Cleanup CSS hot-reload resources
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

void Application::onActivate(GApplication *app, gpointer) {

  // Create main window
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "Activating application, creating MainWindow...");
  auto *window = new MainWindow(ADW_APPLICATION(app));
  window->show();
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO, "MainWindow shown.");

  // Check first run
  if (bwp::config::ConfigManager::getInstance().get<bool>("general.first_run",
                                                          true)) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                            "First run detected - showing Setup Wizard");
    // Assuming MainWindow is a GtkWindow subclass
    auto *wizard = new SetupDialog(window->getGtkWindow());
    wizard->show();
  }
}

void Application::onStartup(GApplication *, gpointer) {
  // Initialize things
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO, "Application startup");

  // Initialize MonitorManager (Wayland connection)
  if (!bwp::monitor::MonitorManager::getInstance().initialize()) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::ERR,
                            "Failed to initialize MonitorManager");
    // Consider fatal? But let's continue for now, maybe user has no monitors?
  }

  // Load CSS with proper path resolution
  loadStylesheet();

  // Setup hot-reload for development
  // Setup hot-reload for development
  setupCssHotReload();

  // Start daemon and tray
  ensureBackgroundServices();
}

std::string Application::findStylesheetPath() {
  std::vector<std::string> searchPaths;

  // 1. XDG data directories (for installed version)
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

  // 2. System data directories
  // 2. System data directories (XDG_DATA_DIRS)
  const char *xdgDataDirs = std::getenv("XDG_DATA_DIRS");
  // Default to /usr/local/share:/usr/share if not set
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

  // 3. Development paths (relative to executable)
  // Get the executable path and construct relative paths from it
  std::filesystem::path exePath;
  try {
    exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
    // From build directory: ../data/ui/style.css
    searchPaths.push_back((exePath / "../data/ui/style.css").string());
    // From build/src/gui: ../../../data/ui/style.css
    searchPaths.push_back((exePath / "../../data/ui/style.css").string());
    searchPaths.push_back((exePath / "../../../data/ui/style.css").string());
  } catch (const std::exception &e) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARN,
                            "Failed to get executable path: " +
                                std::string(e.what()));
  }

  // 4. Current working directory fallbacks
  searchPaths.push_back("data/ui/style.css");
  searchPaths.push_back("../data/ui/style.css");

  // Search for the CSS file
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
      // Path doesn't exist or can't be accessed, continue searching
      (void)e; // Suppress unused variable warning
    }
  }

  return ""; // Not found
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

  // Store the path for hot-reload
  s_currentCssPath = cssPath;

  // Create persistent provider for hot-reload support
  if (!s_cssProvider) {
    s_cssProvider = gtk_css_provider_new();
  }

  gtk_css_provider_load_from_path(s_cssProvider, cssPath.c_str());

  // Check if the provider has content by trying to get display
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::ERR,
                            "Could not get default display for CSS loading");
    return;
  }

  gtk_style_context_add_provider_for_display(
      display, GTK_STYLE_PROVIDER(s_cssProvider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "Successfully loaded stylesheet from: " + cssPath);

  // Initialize WallpaperManager (syncs with MonitorManager)
  bwp::wallpaper::WallpaperManager::getInstance().initialize();

  // Setup Wayland event polling via GLib main loop
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

  // Create a file monitor for the CSS file
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

  // Only reload on content changes, not on attribute changes
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

  // Reload the CSS file - GTK will automatically apply the changes
  gtk_css_provider_load_from_path(s_cssProvider, s_currentCssPath.c_str());

  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "Stylesheet reloaded successfully");
}

void Application::ensureBackgroundServices() {
  // 1. Daemon
  // We could try to connect first, but spawnProcess handles "already running"
  // check if needed? Actually, g_spawn just launches. We should rely on
  // lockfile in daemon. However, we want to allow the daemon to manage itself.
  // Lets just attempt spawn. The daemon handles single-instance logic.
  spawnProcess("betterwallpaper-daemon", "../daemon/betterwallpaper-daemon");

  // 2. Tray
  spawnProcess("betterwallpaper-tray", "../tray/betterwallpaper-tray");
}

bool Application::spawnProcess(const std::string &name,
                               const std::string &relativePath) {
  std::filesystem::path exePath;
  try {
    exePath = std::filesystem::canonical("/proc/self/exe").parent_path();
    std::filesystem::path fullPath = exePath / relativePath;

    // Development path check
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

    // Installed path check (in PATH)
    // If not found relative, assume it's in /usr/bin and G_SPAWN_SEARCH_PATH
    // will找 it if we just pass name? But we want to prefer local build for
    // testing.

    // Fallback to system path
    bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                            "Spawning system " + name);
    char *argv[] = {g_strdup(name.c_str()), nullptr};
    GError *err = nullptr;
    g_spawn_async(nullptr, argv, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr,
                  nullptr, &err);
    g_free(argv[0]);
    if (err) {
      // Suppress error if just testing? No, user wants it to work.
      // But if running installed, it might be started by systemd.
      // We'll log warning but not fail app.
      g_error_free(err);
    }

  } catch (...) {
    return false;
  }
  return true;
}

} // namespace bwp::gui
