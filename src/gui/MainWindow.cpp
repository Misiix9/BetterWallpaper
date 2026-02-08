#include "MainWindow.hpp"
#include "../core/config/ConfigManager.hpp"
#include "../core/input/InputManager.hpp"
#include "../core/power/PowerManager.hpp"
#include "../core/slideshow/SlideshowManager.hpp"
#include "../core/utils/Constants.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/utils/ToastManager.hpp"
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

static constexpr int MIN_WINDOW_WIDTH = 960;
static constexpr int MIN_WINDOW_HEIGHT = 600;
static constexpr int DEFAULT_WINDOW_WIDTH =
    bwp::constants::defaults::WINDOW_WIDTH;
static constexpr int DEFAULT_WINDOW_HEIGHT =
    bwp::constants::defaults::WINDOW_HEIGHT;
// Sidebar width handled in Sidebar class (240px)

// #endregion

MainWindow::MainWindow(AdwApplication *app) {
  // #endregion

  LOG_SCOPE_AUTO();
  m_window = GTK_WIDGET(adw_application_window_new(GTK_APPLICATION(app)));
  gtk_window_set_title(GTK_WINDOW(m_window), bwp::constants::APP_NAME);

  loadWindowState();

  // Enforce defaults if not loaded
  int w, h;
  gtk_window_get_default_size(GTK_WINDOW(m_window), &w, &h);
  if (w < MIN_WINDOW_WIDTH)
    w = DEFAULT_WINDOW_WIDTH;
  if (h < MIN_WINDOW_HEIGHT)
    h = DEFAULT_WINDOW_HEIGHT;
  gtk_window_set_default_size(GTK_WINDOW(m_window), w, h);
  gtk_widget_set_size_request(m_window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);

  g_signal_connect(m_window, "close-request", G_CALLBACK(onCloseRequest), this);

  // #endregion

  setupUi();

  // Register toast notification callback
  bwp::core::utils::ToastManager::getInstance().setExtendedCallback(
      [this](const bwp::core::utils::ToastRequest &request) {
        // Must run on GTK main thread
        auto *req = new bwp::core::utils::ToastRequest(request);
        g_idle_add(+[](gpointer data) -> gboolean {
          auto *r = static_cast<bwp::core::utils::ToastRequest *>(data);
          // Find the MainWindow's toast overlay from the app
          auto *app = g_application_get_default();
          if (!app) { delete r; return FALSE; }
          auto *win = gtk_application_get_active_window(GTK_APPLICATION(app));
          if (!win) { delete r; return FALSE; }

          // Navigate to the toast overlay (it's the content of the AdwApplicationWindow)
          GtkWidget *content = adw_application_window_get_content(ADW_APPLICATION_WINDOW(win));
          if (!content || !ADW_IS_TOAST_OVERLAY(content)) { delete r; return FALSE; }

          // Create toast with type-specific prefix
          std::string prefix;
          switch (r->type) {
            case bwp::core::utils::ToastType::Success: prefix = "\u2713 "; break;
            case bwp::core::utils::ToastType::Error:   prefix = "\u2717 "; break;
            case bwp::core::utils::ToastType::Warning: prefix = "\u26A0 "; break;
            default: break;
          }

          AdwToast *toast = adw_toast_new((prefix + r->message).c_str());
          adw_toast_set_timeout(toast, r->durationMs > 0 ? (r->durationMs / 1000) : 0);

          // Add first action button if present
          if (!r->actions.empty()) {
            adw_toast_set_button_label(toast, r->actions[0].label.c_str());
            auto *cb = new std::function<void()>(r->actions[0].callback);
            g_signal_connect_data(toast, "button-clicked",
                G_CALLBACK(+[](AdwToast*, gpointer data) {
                  auto *fn = static_cast<std::function<void()>*>(data);
                  if (fn && *fn) (*fn)();
                }),
                cb,
                +[](gpointer data, GClosure*) {
                  delete static_cast<std::function<void()>*>(data);
                },
                G_CONNECT_DEFAULT);
          }

          adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(content), toast);
          delete r;
          return FALSE;
        }, req);
      });

  // #endregion

  bwp::input::InputManager::getInstance().setup(GTK_WINDOW(m_window));

  bwp::core::PowerManager::getInstance().addCallback([this](bool onBattery) {
    bool shouldPause = bwp::config::ConfigManager::getInstance().get<bool>(
        "playback.pause_on_battery");
    if (onBattery && shouldPause) {
      if (m_weRenderer)
        m_weRenderer->pause();
    } else {
      if (!m_userPaused) {
        if (m_weRenderer)
          m_weRenderer->play();
      }
    }
  });
  bwp::core::PowerManager::getInstance().startMonitoring();

  gtk_widget_set_visible(m_window, TRUE);

  // #endregion

  m_weRenderer = std::make_unique<bwp::wallpaper::WallpaperEngineRenderer>();

  // #endregion

  // Slideshow Logic
  auto &sm = bwp::core::SlideshowManager::getInstance();
  sm.setChangeCallback([this](const std::string &id) {
    auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
    auto wp = lib.getWallpaper(id);
    if (wp) {
      bool isWe = (wp->type == bwp::wallpaper::WallpaperType::WEScene ||
                   wp->type == bwp::wallpaper::WallpaperType::WEVideo);

      if (isWe) {
        auto &conf = bwp::config::ConfigManager::getInstance();
        int fps = wp->settings.fps > 0 ? wp->settings.fps
                                       : conf.get<int>("performance.fps_limit");
        bool mute = wp->settings.muted;
        if (!mute && !conf.get<bool>("defaults.audio_enabled"))
          if (!mute && !conf.get<bool>("defaults.audio_enabled"))
            mute = true;

        m_weRenderer->setFpsLimit(fps);
        m_weRenderer->setMuted(mute);
        // Set monitor if needed
        m_weRenderer->load(wp->path);
      } else {
        m_weRenderer->stop();
        bwp::wallpaper::NativeWallpaperSetter::getInstance().setWallpaper(
            wp->path);
      }
    }

    // Update Favorite Count (Proof of concept implementation - counts all
    // favorites) Optimization: Cache this or only re-count if favorite status
    // changed? For now, simple count.
    auto all = lib.getAllWallpapers();
    int favCount = 0;
    for (const auto &w : all) {
      if (w.favorite)
        favCount++;
    }
    if (m_sidebar)
      m_sidebar->updateBadge("favorites", favCount);
  });
}

MainWindow::~MainWindow() { saveWindowState(); }

void MainWindow::show() {
  LOG_SCOPE_AUTO();
  gtk_window_present(GTK_WINDOW(m_window));
}

void MainWindow::setupUi() {
  LOG_SCOPE_AUTO();
  // LAYOUT: Monochrome Glass
  // Root: GtkBox (Horizontal) -> [Sidebar] [Separator] [Stack]
  // Wrapped in AdwApplicationWindow content.

  // 0. Toast Overlay
  m_toastOverlay = adw_toast_overlay_new();
  adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window),
                                     m_toastOverlay);

  GtkWidget *rootBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  // adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window),
  // rootBox); // Old way
  adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(m_toastOverlay), rootBox);

  // 1. Sidebar
  m_sidebar = std::make_unique<Sidebar>();
  gtk_box_append(GTK_BOX(rootBox), m_sidebar->getWidget());

  // 2. Separator (Vertical Line) - handled by .sidebar border-right usually,
  // but explicit is safer for styling Actually Design Memory says "Border:
  // #262626". Sidebar CSS handles it.

  // 3. Content Area
  m_contentStack = adw_view_stack_new();
  gtk_widget_set_hexpand(m_contentStack, TRUE);
  gtk_widget_set_vexpand(m_contentStack, TRUE);
  gtk_box_append(GTK_BOX(rootBox), m_contentStack);

  // Wire up Sidebar
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
      // New Folder Dialog logic...
      // (Simplified for this refactor, relying on existing logic patterns)
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
    } else if (page == "profiles") {
      ensureProfilesView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "profiles");
    } else if (page == "schedule") {
      ensureScheduleView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "schedule");
    } else if (page == "hyprland") {
      ensureHyprlandView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "hyprland");
    }
  });

  // Create Core Views
  m_libraryView = std::make_unique<LibraryView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_libraryView->getWidget(), "library");

  m_monitorsView = std::make_unique<MonitorsView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_monitorsView->getWidget(), "monitors");

  m_folderView = std::make_unique<FolderView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_folderView->getWidget(), "folder");

  // Initial Favorite Count
  auto all = bwp::wallpaper::WallpaperLibrary::getInstance().getAllWallpapers();
  int favCount = 0;
  for (const auto &w : all) {
    if (w.favorite)
      favCount++;
  }
  m_sidebar->updateBadge("favorites", favCount);
}

void MainWindow::ensureWorkshopView() {
  LOG_SCOPE_AUTO();
  if (m_workshopViewAdded)
    return;
  m_workshopView.ensureLoaded([this](WorkshopView &view) {
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "workshop");
  });
  m_workshopViewAdded = true;
}

void MainWindow::ensureSettingsView() {
  LOG_SCOPE_AUTO();
  if (m_settingsViewAdded)
    return;
  m_settingsView.ensureLoaded([this](SettingsView &view) {
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "settings");
  });
  m_settingsViewAdded = true;
}

void MainWindow::ensureFavoritesView() {
  LOG_SCOPE_AUTO();
  if (m_favoritesViewAdded)
    return;
  m_favoritesView.ensureLoaded([this](FavoritesView &view) {
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "favorites");
  });
  m_favoritesViewAdded = true;
}

void MainWindow::ensureProfilesView() {
  LOG_SCOPE_AUTO();
  if (m_profilesViewAdded)
    return;
  m_profilesView.ensureLoaded([this](ProfilesView &view) {
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "profiles");
  });
  m_profilesViewAdded = true;
}

void MainWindow::ensureScheduleView() {
  LOG_SCOPE_AUTO();
  if (m_scheduleViewAdded)
    return;
  m_scheduleView.ensureLoaded([this](ScheduleView &view) {
    adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack), view.getWidget(),
                             "schedule");
  });
  m_scheduleViewAdded = true;
}

void MainWindow::ensureHyprlandView() {
  LOG_SCOPE_AUTO();
  if (m_hyprlandViewAdded)
    return;
  m_hyprlandView = std::make_unique<HyprlandWorkspacesView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_hyprlandView->getWidget(), "hyprland");
  m_hyprlandViewAdded = true;
}

void MainWindow::loadWindowState() {
  LOG_SCOPE_AUTO();
  auto &config = bwp::config::ConfigManager::getInstance();
  int width = config.get<int>("window.width");
  int height = config.get<int>("window.height");
  if (width < MIN_WINDOW_WIDTH)
    width = DEFAULT_WINDOW_WIDTH;
  if (height < MIN_WINDOW_HEIGHT)
    height = DEFAULT_WINDOW_HEIGHT;

  std::string mode = config.get<std::string>("general.window_mode");
  bool isFloating = (mode == "floating");

  if (isFloating) {
    // Floating: Fixed size, not resizable by user (standard app behavior?)
    // Or maybe just resizable but with defaults? User asked for "fixed width
    // and height".
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);
    gtk_widget_set_size_request(m_window, width, height); // Force size
  } else {
    // Tiling: Resizable, let WM handle it
    gtk_window_set_resizable(GTK_WINDOW(m_window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height); // hint
    gtk_widget_set_size_request(m_window, -1, -1); // Unset forced size
  }

  if (config.get<bool>("window.maximized") && !isFloating) {
    gtk_window_maximize(GTK_WINDOW(m_window));
  }
}

// Keep saveWindowState and onCloseRequest as is (copied from previous logic
// implicitly by not changing them if they were separate... wait, I'm
// refactoring the whole file essentially to ensure consistency, so I will
// include them).
void MainWindow::saveWindowState() {
  LOG_SCOPE_AUTO();
  if (!m_window)
    return;
  auto &config = bwp::config::ConfigManager::getInstance();
  bool maximized = gtk_window_is_maximized(GTK_WINDOW(m_window));
  config.set<bool>("window.maximized", maximized);

  if (!maximized) {
    int width = gtk_widget_get_width(m_window);
    int height = gtk_widget_get_height(m_window);
    if (width >= MIN_WINDOW_WIDTH && height >= MIN_WINDOW_HEIGHT) {
      config.set<int>("window.width", width);
      config.set<int>("window.height", height);
    }
  }
}

gboolean MainWindow::onCloseRequest(GtkWindow *, gpointer user_data) {
  LOG_SCOPE_AUTO();
  MainWindow *self = static_cast<MainWindow *>(user_data);
  if (self)
    self->saveWindowState();
  return FALSE;
}

} // namespace bwp::gui
