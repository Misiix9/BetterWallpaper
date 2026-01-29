#include "MainWindow.hpp"
#include "../core/config/ConfigManager.hpp"
#include "../core/utils/Logger.hpp"
#include "views/FavoritesView.hpp"
#include "views/RecentView.hpp"
#include "views/SettingsView.hpp"
#include <adwaita.h>

namespace bwp::gui {

static constexpr int MIN_WINDOW_WIDTH = 800;
static constexpr int MIN_WINDOW_HEIGHT = 600;
static constexpr int DEFAULT_WINDOW_WIDTH = 1280;
static constexpr int DEFAULT_WINDOW_HEIGHT = 720;
static constexpr int SIDEBAR_WIDTH = 200;

MainWindow::MainWindow(AdwApplication *app) {
  // Use adw_application_window which gives nice rounded corners
  m_window = GTK_WIDGET(adw_application_window_new(GTK_APPLICATION(app)));

  gtk_window_set_title(GTK_WINDOW(m_window), "BetterWallpaper");

  // Load saved window size or use defaults
  loadWindowState();

  // Set minimum size constraints
  gtk_widget_set_size_request(m_window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);

  // Enable window resizing for proper user experience (default, overriden by
  // config)
  gtk_window_set_resizable(GTK_WINDOW(m_window), TRUE);

  // Connect close request to save window state
  g_signal_connect(m_window, "close-request", G_CALLBACK(onCloseRequest), this);

  setupUi();

  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "MainWindow initialized and ready (resizable)");
}

MainWindow::~MainWindow() {
  // Save window state on destruction
  saveWindowState();
}

void MainWindow::show() { gtk_window_present(GTK_WINDOW(m_window)); }

void MainWindow::setupUi() {
#if ADW_CHECK_VERSION(1, 4, 0)
  m_splitView = adw_overlay_split_view_new();
  adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window),
                                     m_splitView);

  m_sidebar = std::make_unique<Sidebar>();
  GtkWidget *sidebarWidget = m_sidebar->getWidget();
  gtk_widget_set_size_request(sidebarWidget, SIDEBAR_WIDTH, -1);
  adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(m_splitView),
                                     sidebarWidget);
  adw_overlay_split_view_set_sidebar_width_fraction(
      ADW_OVERLAY_SPLIT_VIEW(m_splitView), 0.15);

  m_sidebar->setCallback([this](const std::string &page) {
    if (page == "library") {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "library");
    } else if (page == "monitors") {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "monitors");
      m_monitorsView->refresh();
    } else if (page == "workshop") {
      ensureWorkshopView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "workshop");
    } else if (page.rfind("folder.", 0) == 0) {
      std::string folderId = page.substr(7);
      m_folderView->loadFolder(folderId);
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "folder");
    } else if (page == "settings") {
      ensureSettingsView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "settings");
    } else if (page == "favorites") {
      ensureFavoritesView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "favorites");
    } else if (page == "recent") {
      ensureRecentView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "recent");
    }
  });

  m_contentStack = adw_view_stack_new();
  adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(m_splitView),
                                     m_contentStack);

  // Create core views immediately (always needed)
  LOG_INFO("Creating LibraryView...");
  m_libraryView = std::make_unique<LibraryView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_libraryView->getWidget(), "library");

  LOG_INFO("Creating MonitorsView...");
  m_monitorsView = std::make_unique<MonitorsView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_monitorsView->getWidget(), "monitors");

  LOG_INFO("Creating FolderView...");
  m_folderView = std::make_unique<FolderView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_folderView->getWidget(), "folder");

  // Note: Workshop, Settings, Favorites, Recent views are lazy-loaded
  LOG_INFO("Views initialized (4 lazy-loaded views deferred)");

#else
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window), box);

  m_sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(m_sidebar, SIDEBAR_WIDTH, -1);
  gtk_box_append(GTK_BOX(box), m_sidebar);

  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
  gtk_box_append(GTK_BOX(box), separator);

  m_contentStack = gtk_stack_new();
  gtk_box_append(GTK_BOX(box), m_contentStack);

  m_libraryView = std::make_unique<LibraryView>();
  gtk_stack_add_named(GTK_STACK(m_contentStack), m_libraryView->getWidget(),
                      "library");

  m_monitorsView = std::make_unique<MonitorsView>();
  gtk_stack_add_named(GTK_STACK(m_contentStack), m_monitorsView->getWidget(),
                      "monitors");

  m_folderView = std::make_unique<FolderView>();
  gtk_stack_add_named(GTK_STACK(m_contentStack), m_folderView->getWidget(),
                      "folder");
#endif
}

void MainWindow::ensureWorkshopView() {
  if (m_workshopViewAdded)
    return;

  m_workshopView.ensureLoaded([this](WorkshopView &view) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "workshop");
#else
    gtk_stack_add_named(GTK_STACK(m_contentStack), view.getWidget(),
                        "workshop");
#endif
  });

  m_workshopViewAdded = true;
}

void MainWindow::ensureSettingsView() {
  if (m_settingsViewAdded)
    return;

  m_settingsView.ensureLoaded([this](SettingsView &view) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "settings");
#else
    gtk_stack_add_named(GTK_STACK(m_contentStack), view.getWidget(),
                        "settings");
#endif
  });

  m_settingsViewAdded = true;
}

void MainWindow::ensureFavoritesView() {
  if (m_favoritesViewAdded)
    return;

  m_favoritesView.ensureLoaded([this](FavoritesView &view) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "favorites");
#else
    gtk_stack_add_named(GTK_STACK(m_contentStack), view.getWidget(),
                        "favorites");
#endif
  });

  m_favoritesViewAdded = true;
}

void MainWindow::ensureRecentView() {
  if (m_recentViewAdded)
    return;

  m_recentView.ensureLoaded([this](RecentView &view) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "recent");
#else
    gtk_stack_add_named(GTK_STACK(m_contentStack), view.getWidget(), "recent");
#endif
  });

  m_recentViewAdded = true;
}

void MainWindow::loadWindowState() {
  auto &config = bwp::config::ConfigManager::getInstance();

  // Try to get saved window dimensions
  int width = config.get<int>("window.width");
  int height = config.get<int>("window.height");

  // Use defaults if not saved or invalid
  if (width < MIN_WINDOW_WIDTH || height < MIN_WINDOW_HEIGHT) {
    width = DEFAULT_WINDOW_WIDTH;
    height = DEFAULT_WINDOW_HEIGHT;
  }

  // Check window mode
  std::string mode = config.get<std::string>("general.window_mode");
  bool isFloating = (mode == "floating");

  if (isFloating) {
    // Fixed size, not resizable
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
  } else {
    // Tiling/Resizable
    gtk_window_set_resizable(GTK_WINDOW(m_window), TRUE);
  }

  gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);

  // Check if window was maximized
  bool maximized = config.get<bool>("window.maximized");
  if (maximized && !isFloating) {
    gtk_window_maximize(GTK_WINDOW(m_window));
  }

  bwp::utils::Logger::log(bwp::utils::LogLevel::DEBUG,
                          "Loaded window state: " + std::to_string(width) +
                              "x" + std::to_string(height) +
                              (maximized ? " (maximized)" : "") +
                              " Mode: " + mode);
}

void MainWindow::saveWindowState() {
  if (!m_window)
    return;

  auto &config = bwp::config::ConfigManager::getInstance();

  // Check if maximized - don't save dimensions if maximized
  bool maximized = gtk_window_is_maximized(GTK_WINDOW(m_window));
  config.set<bool>("window.maximized", maximized);

  if (!maximized) {
    // Get current window dimensions
    int width = gtk_widget_get_width(m_window);
    int height = gtk_widget_get_height(m_window);

    // Only save valid dimensions
    if (width >= MIN_WINDOW_WIDTH && height >= MIN_WINDOW_HEIGHT) {
      config.set<int>("window.width", width);
      config.set<int>("window.height", height);

      bwp::utils::Logger::log(bwp::utils::LogLevel::DEBUG,
                              "Saved window state: " + std::to_string(width) +
                                  "x" + std::to_string(height));
    }
  }
}

gboolean MainWindow::onCloseRequest(GtkWindow *window, gpointer user_data) {
  (void)window; // Unused parameter
  MainWindow *self = static_cast<MainWindow *>(user_data);
  if (self) {
    self->saveWindowState();
  }

  // Return FALSE to allow the window to close
  // Return TRUE to prevent closing (e.g., for "minimize to tray" feature)
  return FALSE;
}

} // namespace bwp::gui
