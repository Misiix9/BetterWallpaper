#include "MainWindow.hpp"
#include "../core/utils/Logger.hpp"
#include "views/FavoritesView.hpp"
#include "views/RecentView.hpp"
#include "views/SettingsView.hpp"
#include <adwaita.h>

namespace bwp::gui {

static constexpr int MIN_WINDOW_WIDTH = 1000;
static constexpr int MIN_WINDOW_HEIGHT = 600;
static constexpr int DEFAULT_WINDOW_WIDTH = 1280;
static constexpr int DEFAULT_WINDOW_HEIGHT = 720;
static constexpr int SIDEBAR_WIDTH = 200;

MainWindow::MainWindow(AdwApplication *app) {
  // Use adw_application_window which gives nice rounded corners
  m_window = GTK_WIDGET(adw_application_window_new(GTK_APPLICATION(app)));

  gtk_window_set_title(GTK_WINDOW(m_window), "BetterWallpaper");
  gtk_window_set_default_size(GTK_WINDOW(m_window), DEFAULT_WINDOW_WIDTH,
                              DEFAULT_WINDOW_HEIGHT);
  gtk_widget_set_size_request(m_window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);

  // Set window role/class to help Hyprland rules
  // Setting DIALOG hint usually forces tiling WMs to float the window
  // gtk_window_set_type_hint(GTK_WINDOW(m_window),
  // GDK_SURFACE_TYPE_HINT_DIALOG);

  // Make window fixed size (dialog-like) to encourage floating behavior
  // and prevent layout breakage
  gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);

  setupUi();

  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO,
                          "MainWindow initialized and ready");
}

MainWindow::~MainWindow() {}

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
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "workshop");
    } else if (page.rfind("folder.", 0) == 0) {
      std::string folderId = page.substr(7);
      m_folderView->loadFolder(folderId);
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "folder");
    } else if (page == "settings") {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "settings");
    } else if (page == "favorites") {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "favorites");
    } else if (page == "recent") {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "recent");
    }
  });

  m_contentStack = adw_view_stack_new();
  adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(m_splitView),
                                     m_contentStack);

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

  LOG_INFO("Creating WorkshopView...");
  m_workshopView = std::make_unique<WorkshopView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_workshopView->getWidget(), "workshop");

  LOG_INFO("Creating SettingsView...");
  m_settingsView = std::make_unique<SettingsView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_settingsView->getWidget(), "settings");

  LOG_INFO("Creating FavoritesView...");
  m_favoritesView = std::make_unique<FavoritesView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_favoritesView->getWidget(), "favorites");

  LOG_INFO("Creating RecentView...");
  m_recentView = std::make_unique<RecentView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_recentView->getWidget(), "recent");

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

} // namespace bwp::gui
