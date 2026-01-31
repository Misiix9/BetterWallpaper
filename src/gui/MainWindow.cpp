#include "MainWindow.hpp"
#include "../core/config/ConfigManager.hpp"
#include "../core/input/InputManager.hpp"
#include "../core/power/PowerManager.hpp"
#include "../core/slideshow/SlideshowManager.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/wallpaper/FolderManager.hpp"
#include "../core/wallpaper/NativeWallpaperSetter.hpp"
#include "../core/wallpaper/WallpaperLibrary.hpp"
#include "../core/wallpaper/renderers/WallpaperEngineRenderer.hpp"
#include "views/FavoritesView.hpp"
#include "views/ProfilesView.hpp"

#include "views/ScheduleView.hpp"
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
  // loadWindowState(); // DEBUG: Disable state loading

  // Force default size
  gtk_window_set_default_size(GTK_WINDOW(m_window), DEFAULT_WINDOW_WIDTH,
                              DEFAULT_WINDOW_HEIGHT);

  // Set minimum size constraints
  gtk_widget_set_size_request(m_window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);

  // Enable window resizing
  gtk_window_set_resizable(GTK_WINDOW(m_window), TRUE);

  // Connect close request to save window state
  // g_signal_connect(m_window, "close-request", G_CALLBACK(onCloseRequest),
  // this);

  setupUi();

  // Setup Input
  bwp::input::InputManager::getInstance().setup(GTK_WINDOW(m_window));

  // Setup Power Manager
  bwp::core::PowerManager::getInstance().addCallback([this](bool onBattery) {
    bool shouldPause = bwp::config::ConfigManager::getInstance().get<bool>(
        "playback.pause_on_battery");
    if (onBattery && shouldPause) {
      bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                              "Power: Pausing on battery");
      if (m_weRenderer)
        m_weRenderer->pause(); // Assuming pause() exists, need to verify
    } else {
      bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                              "Power: Resuming (AC or Setting Disabled)");
      if (m_weRenderer)
        m_weRenderer->play();
    }
  });
  bwp::core::PowerManager::getInstance().startMonitoring();

  gtk_widget_set_visible(m_window, TRUE); // DEBUG: Force visible

  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "MainWindow initialized and ready (resizable)");

  // Initialize WE Renderer
  m_weRenderer = std::make_unique<bwp::wallpaper::WallpaperEngineRenderer>();

  // Setup Slideshow Callback
  auto &sm = bwp::core::SlideshowManager::getInstance();
  sm.setChangeCallback([this](const std::string &id) {
    auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
    auto wp = lib.getWallpaper(id);

    if (wp) {
      LOG_INFO("Applying wallpaper from slideshow: " + wp->path);
      bool isWe = (wp->type == bwp::wallpaper::WallpaperType::WEScene ||
                   wp->type == bwp::wallpaper::WallpaperType::WEVideo);

      if (isWe) {
        // Gather Settings
        auto &conf = bwp::config::ConfigManager::getInstance();

        // FPS
        int fps = wp->settings.fps; // Override
        if (fps <= 0)
          fps = conf.get<int>("performance.fps_limit");

        // Mute
        bool mute = wp->settings.muted;
        if (!mute) {
          // If not overridden to mute, check global
          // "audio_enabled" means we WANT audio. So if false, we mute.
          bool audioEnabled = conf.get<bool>("defaults.audio_enabled");
          if (!audioEnabled)
            mute = true;
        }

        m_weRenderer->setFpsLimit(fps);
        m_weRenderer->setMuted(mute);

        // Detect monitor for WE
        auto monitors =
            bwp::wallpaper::NativeWallpaperSetter::getInstance().getMonitors();
        if (!monitors.empty()) {
          m_weRenderer->setMonitor(monitors[0]); // Target primary/first for now
        }

        m_weRenderer->load(wp->path);
      } else {
        m_weRenderer->stop();
        bwp::wallpaper::NativeWallpaperSetter::getInstance().setWallpaper(
            wp->path);
      }
    }
  });
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
    } else if (page == "folder.new") {
      // Show dialog to create folder
      GtkWidget *dialog = gtk_dialog_new_with_buttons(
          "Create Folder", GTK_WINDOW(m_window),
          static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL |
                                      GTK_DIALOG_DESTROY_WITH_PARENT),
          "Cancel", GTK_RESPONSE_CANCEL, "Create", GTK_RESPONSE_ACCEPT, NULL);

      GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
      GtkWidget *entry = gtk_entry_new();
      gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Folder Name");
      gtk_widget_set_margin_top(entry, 20);
      gtk_widget_set_margin_bottom(entry, 20);
      gtk_widget_set_margin_start(entry, 20);
      gtk_widget_set_margin_end(entry, 20);
      gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
      gtk_box_append(GTK_BOX(contentArea), entry);

      gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

      g_signal_connect(
          dialog, "response",
          G_CALLBACK(+[](GtkDialog *d, int response, gpointer data) {
            MainWindow *self = static_cast<MainWindow *>(data);
            GtkWidget *entryWidget = static_cast<GtkWidget *>(
                g_object_get_data(G_OBJECT(d), "entry"));

            if (response == GTK_RESPONSE_ACCEPT) {
              const char *name =
                  gtk_editable_get_text(GTK_EDITABLE(entryWidget));
              if (name && strlen(name) > 0) {
                std::string newId =
                    bwp::wallpaper::FolderManager::getInstance().createFolder(
                        name);
                self->m_sidebar->refresh();

                // Navigate to new folder
                self->m_folderView->loadFolder(newId);
#if ADW_CHECK_VERSION(1, 4, 0)
                adw_view_stack_set_visible_child_name(
                    ADW_VIEW_STACK(self->m_contentStack), "folder");
#else
                gtk_stack_set_visible_child_name(
                    GTK_STACK(self->m_contentStack), "folder");
#endif
              }
            }
            gtk_window_destroy(GTK_WINDOW(d));
          }),
          this);

      g_object_set_data(G_OBJECT(dialog), "entry", entry);
      gtk_widget_show(dialog);

    } else if (page.rfind("folder.", 0) == 0) {
      std::string folderId = page.substr(7);
      m_folderView->loadFolder(folderId);
#if ADW_CHECK_VERSION(1, 4, 0)
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "folder");
#else
      gtk_stack_set_visible_child_name(GTK_STACK(m_contentStack), "folder");
#endif
    } else if (page == "settings") {
      ensureSettingsView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "settings");
    } else if (page == "favorites") {
      ensureFavoritesView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "favorites");

    } else if (page == "profiles") {
      ensureProfilesView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "profiles");
    } else if (page == "schedule") {
      ensureScheduleView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "schedule");
    }
  });

  m_contentStack = adw_view_stack_new();

  // Note: AdwViewStack doesn't support direct transition settings like GtkStack
  // Transitions are handled automatically by libadwaita

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

  // Enable slide transitions between views
  gtk_stack_set_transition_type(GTK_STACK(m_contentStack),
                                GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
  gtk_stack_set_transition_duration(GTK_STACK(m_contentStack), 200);

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

void MainWindow::ensureProfilesView() {
  if (m_profilesViewAdded)
    return;

  m_profilesView.ensureLoaded([this](ProfilesView &view) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "profiles");
#else
    gtk_stack_add_named(GTK_STACK(m_contentStack), view.getWidget(),
                        "profiles");
#endif
  });

  m_profilesViewAdded = true;
}

void MainWindow::ensureScheduleView() {
  if (m_scheduleViewAdded)
    return;

  m_scheduleView.ensureLoaded([this](ScheduleView &view) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "schedule");
#else
    gtk_stack_add_named(GTK_STACK(m_contentStack), view.getWidget(),
                        "schedule");
#endif
  });

  m_scheduleViewAdded = true;
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
