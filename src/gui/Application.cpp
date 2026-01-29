#include "Application.hpp"
#include "../core/utils/Logger.hpp"
#include "MainWindow.hpp"
#include <iostream>

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
                              G_APPLICATION_DEFAULT_FLAGS);
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
  // We maintain MainWindow instance via userdata or singleton or just new?
  // Usually we attach it to the application.
  // For simplicity, create and show.
  auto *window = new MainWindow(ADW_APPLICATION(app));
  window->show();
}

void Application::onStartup(GApplication *, gpointer) {
  // Initialize things
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO, "Application startup");

  // Load CSS with proper path resolution
  loadStylesheet();

  // Setup hot-reload for development
  setupCssHotReload();
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
  const char *xdgDataDirs = std::getenv("XDG_DATA_DIRS");
  if (xdgDataDirs && std::strlen(xdgDataDirs) > 0) {
    std::string dirs(xdgDataDirs);
    size_t pos = 0;
    while ((pos = dirs.find(':')) != std::string::npos) {
      std::string dir = dirs.substr(0, pos);
      searchPaths.push_back(dir + "/betterwallpaper/ui/style.css");
      dirs.erase(0, pos + 1);
    }
    if (!dirs.empty()) {
      searchPaths.push_back(dirs + "/betterwallpaper/ui/style.css");
    }
  } else {
    searchPaths.push_back("/usr/local/share/betterwallpaper/ui/style.css");
    searchPaths.push_back("/usr/share/betterwallpaper/ui/style.css");
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
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARNING,
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
        bwp::utils::LogLevel::ERROR,
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
    bwp::utils::Logger::log(bwp::utils::LogLevel::ERROR,
                            "Could not get default display for CSS loading");
    return;
  }

  gtk_style_context_add_provider_for_display(
      display, GTK_STYLE_PROVIDER(s_cssProvider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "Successfully loaded stylesheet from: " + cssPath);
}

void Application::setupCssHotReload() {
  if (s_currentCssPath.empty()) {
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARNING,
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
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARNING,
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
    bwp::utils::Logger::log(bwp::utils::LogLevel::WARNING,
                            "Cannot reload stylesheet: no CSS file loaded");
    return;
  }

  // Reload the CSS file - GTK will automatically apply the changes
  gtk_css_provider_load_from_path(s_cssProvider, s_currentCssPath.c_str());

  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "Stylesheet reloaded successfully");
}

} // namespace bwp::gui
