#include "MainWindow.hpp"
#include "../core/config/ConfigManager.hpp"
#include "../core/input/InputManager.hpp"
#include "../core/ipc/LinuxIPCClient.hpp"
#include "../core/monitor/MonitorManager.hpp"
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
MainWindow::MainWindow(AdwApplication *app) {
  LOG_SCOPE_AUTO();
  m_window = GTK_WIDGET(adw_application_window_new(GTK_APPLICATION(app)));

  // Manage lifetime: delete wrapper when widget is destroyed
  g_signal_connect(m_window, "destroy",
                   G_CALLBACK(+[](GtkWidget *, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     delete self;
                   }),
                   this);

  gtk_window_set_title(GTK_WINDOW(m_window), bwp::constants::APP_NAME);
  loadWindowState();
  int w, h;
  gtk_window_get_default_size(GTK_WINDOW(m_window), &w, &h);

  // sanity check dimensions
  if (w < MIN_WINDOW_WIDTH)
    w = DEFAULT_WINDOW_WIDTH;
  if (h < MIN_WINDOW_HEIGHT)
    h = DEFAULT_WINDOW_HEIGHT;

  gtk_window_set_default_size(GTK_WINDOW(m_window), w, h);
  gtk_widget_set_size_request(m_window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);
  g_signal_connect(m_window, "close-request", G_CALLBACK(onCloseRequest), this);
  // Store MainWindow pointer on the GtkWindow so other widgets can access it
  g_object_set_data(G_OBJECT(m_window), "main-window", this);
  // Self-delete when GTK destroys the window (prevents C++ object leak)
  g_object_weak_ref(
      G_OBJECT(m_window),
      [](gpointer data, GObject *) { delete static_cast<MainWindow *>(data); },
      this);
  setupUi();

  // global toast handler
  bwp::core::utils::ToastManager::getInstance().setExtendedCallback(
      [this](const bwp::core::utils::ToastRequest &request) {
        auto *req = new bwp::core::utils::ToastRequest(request);
        g_idle_add(
            +[](gpointer data) -> gboolean {
              auto *r = static_cast<bwp::core::utils::ToastRequest *>(data);
              auto *app = g_application_get_default();
              if (!app) {
                delete r;
                return FALSE;
              }
              auto *win =
                  gtk_application_get_active_window(GTK_APPLICATION(app));
              if (!win) {
                delete r;
                return FALSE;
              }
              GtkWidget *content = adw_application_window_get_content(
                  ADW_APPLICATION_WINDOW(win));
              if (!content || !ADW_IS_TOAST_OVERLAY(content)) {
                delete r;
                return FALSE;
              }
              std::string prefix;
              std::string textColor;
              switch (r->type) {
              case bwp::core::utils::ToastType::Success:
                prefix = "✓ ";
                textColor = "#86EFAC";
                break;
              case bwp::core::utils::ToastType::Error:
                prefix = "✗ ";
                textColor = "#FCA5A5";
                break;
              case bwp::core::utils::ToastType::Warning:
                prefix = "⚠ ";
                textColor = "#FCD34D";
                break;
              default:
                break;
              }
              // libadwaita asserts title != NULL even if custom title is used
              AdwToast *toast = adw_toast_new(" ");
              adw_toast_set_timeout(
                  toast, r->durationMs > 0 ? (r->durationMs / 1000) : 0);
              GtkWidget *toastLabel = gtk_label_new(nullptr);
              std::string fullMsg = prefix + r->message;
              if (!textColor.empty()) {
                char *escaped = g_markup_escape_text(fullMsg.c_str(), -1);
                std::string markup =
                    "<span color='" + textColor + "'>" + escaped + "</span>";
                gtk_label_set_markup(GTK_LABEL(toastLabel), markup.c_str());
                g_free(escaped);
              } else {
                gtk_label_set_text(GTK_LABEL(toastLabel), fullMsg.c_str());
              }
              gtk_label_set_ellipsize(GTK_LABEL(toastLabel),
                                      PANGO_ELLIPSIZE_END);
              adw_toast_set_custom_title(toast, toastLabel);
              if (!r->actions.empty()) {
                adw_toast_set_button_label(toast, r->actions[0].label.c_str());
                auto *cb = new std::function<void()>(r->actions[0].callback);
                g_signal_connect_data(
                    toast, "button-clicked",
                    G_CALLBACK(+[](AdwToast *, gpointer data) {
                      auto *fn = static_cast<std::function<void()> *>(data);
                      if (fn && *fn)
                        (*fn)();
                    }),
                    cb,
                    +[](gpointer data, GClosure *) {
                      delete static_cast<std::function<void()> *>(data);
                    },
                    G_CONNECT_DEFAULT);
              }
              adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(content), toast);
              delete r;
              return FALSE;
            },
            req);
      });
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
  m_weRenderer = std::make_unique<bwp::wallpaper::WallpaperEngineRenderer>();
  auto &sm = bwp::core::SlideshowManager::getInstance();
  sm.setChangeCallback([this](const std::string &id) {
    auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
    auto wp = lib.getWallpaper(id);
    if (wp) {
      auto &monMgr = bwp::monitor::MonitorManager::getInstance();
      auto monitors = monMgr.getMonitors();
      if (monitors.empty()) {
        LOG_WARN("Slideshow: no monitors detected, cannot apply");
        return;
      }
      bwp::ipc::LinuxIPCClient ipcClient;
      if (ipcClient.connect()) {
        // Apply to ALL monitors so every screen gets the new wallpaper
        for (const auto &mon : monitors) {
          LOG_INFO("Slideshow: sending IPC SetWallpaper on " + mon.name +
                   ": " + wp->path);
          ipcClient.setWallpaper(wp->path, mon.name);
        }
      } else {
        LOG_ERROR("Slideshow: failed to connect to daemon via IPC");
      }
    }
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
MainWindow::~MainWindow() {
  // saveWindowState(); // Called in onCloseRequest. Avoiding access during destruction.
}
void MainWindow::show() {
  LOG_SCOPE_AUTO();
  gtk_window_present(GTK_WINDOW(m_window));
}
void MainWindow::navigateTo(const std::string &page) {
  LOG_INFO("Navigating to page: " + page);
  if (m_sidebar) {
    m_sidebar->select(page);
    // The sidebar selection triggers the callback, which switches the view.
    // However, if the page is already selected, the callback might not fire
    // (depending on GtkListBox behavior). So we manually trigger the view
    // switch logic here just in case, or we rely on sidebar->select triggering
    // row-activated. Sidebar::select calls gtk_list_box_select_row which emits
    // row-activated. But wait, gtk_list_box_select_row emits row-activated only
    // if the selection actually changes. If we are already on that page, we
    // should ensure the window is presented (handled by Application) and maybe
    // refresh if needed. For now, let's assume sidebar selection is enough.
    // Actually, let's FORCE the view switch by calling the callback manually if
    // needed, but the callback is internal context. Let's just trust
    // Sidebar::select for now or extract the switch logic. Better: let's
    // extract the switch logic to a private method 'switchToPage' and call it.
    // Refactoring m_sidebar->setCallback lambda to a member function would be
    // cleaner, but for now: We can just duplicate the simplified switch logic
    // or call the lambda if we stored it? See setupUi for how the lambda is
    // defined. Actually, `gtk_list_box_select_row` DOES emits `row-activated`
    // signal even if same row is selected? No, usually only on change. Let's
    // modify Sidebar::select to force emit or just call the logic. To be safe
    // and simple: valid pages are "settings", "library", "workshop",
    // "monitors".
    if (page == "library") {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "library");
    } else if (page == "monitors") {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "monitors");
    } else if (page == "workshop") {
      ensureWorkshopView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "workshop");
    } else if (page == "settings") {
      ensureSettingsView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "settings");
    } else if (page == "favorites") {
      ensureFavoritesView();
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "favorites");
    }
  }
}
void MainWindow::setupUi() {
  LOG_SCOPE_AUTO();
  m_toastOverlay = adw_toast_overlay_new();
  adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window),
                                     m_toastOverlay);
  GtkWidget *rootBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(m_toastOverlay), rootBox);
  m_sidebar = std::make_unique<Sidebar>();
  gtk_box_append(GTK_BOX(rootBox), m_sidebar->getWidget());
  m_contentStack = adw_view_stack_new();
  gtk_widget_set_hexpand(m_contentStack, TRUE);
  gtk_widget_set_vexpand(m_contentStack, TRUE);

  // Wrap content stack in an overlay so DownloadIndicator can float on top
  GtkWidget *contentOverlay = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(contentOverlay), m_contentStack);

  m_downloadIndicator = std::make_unique<DownloadIndicator>();
  gtk_overlay_add_overlay(GTK_OVERLAY(contentOverlay),
                          m_downloadIndicator->getWidget());

  // Connect DownloadQueue to Indicator
  auto &dq = bwp::steam::DownloadQueue::getInstance();
  dq.setQueueChangeCallback(
      [this](const std::vector<bwp::steam::QueueItem> &queue) {
        // Run on main thread
        std::vector<bwp::steam::QueueItem> copy = queue;
        g_idle_add(
            +[](gpointer data) -> gboolean {
              auto *pkg = static_cast<std::pair<
                  MainWindow *, std::vector<bwp::steam::QueueItem>> *>(data);
              if (pkg->first->m_downloadIndicator) {
                pkg->first->m_downloadIndicator->updateFromQueue(pkg->second);
              }
              delete pkg;
              return FALSE;
            },
            new std::pair<MainWindow *, std::vector<bwp::steam::QueueItem>>(
                this, copy));
      });

  // Connect Retry Logic
  m_downloadIndicator->onRetry([](const std::string &id) {
    bwp::steam::DownloadQueue::getInstance().addToQueue(id, "");
  });

  gtk_box_append(GTK_BOX(rootBox), contentOverlay);
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
  m_libraryView = std::make_unique<LibraryView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_libraryView->getWidget(), "library");
  m_monitorsView = std::make_unique<MonitorsView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_monitorsView->getWidget(), "monitors");
  m_folderView = std::make_unique<FolderView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_folderView->getWidget(), "folder");
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
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);
    gtk_widget_set_size_request(m_window, width, height);
  } else {
    gtk_window_set_resizable(GTK_WINDOW(m_window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);
    gtk_widget_set_size_request(m_window, -1, -1);
  }
  if (config.get<bool>("window.maximized") && !isFloating) {
    gtk_window_maximize(GTK_WINDOW(m_window));
  }
}
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
