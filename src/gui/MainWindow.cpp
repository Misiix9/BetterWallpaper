#include "MainWindow.hpp"
#include "../utils/Logger.hpp"

namespace bwp::gui {

MainWindow::MainWindow(AdwApplication *app) {
  m_window = GTK_WIDGET(adw_application_window_new(app));
  gtk_window_set_title(GTK_WINDOW(m_window), "BetterWallpaper");
  gtk_window_set_default_size(GTK_WINDOW(m_window), 1200, 800);

  setupUi();
}

MainWindow::~MainWindow() {
  // Widgets are destroyed by GTK hierarchy
}

void MainWindow::show() { gtk_window_present(GTK_WINDOW(m_window)); }

void MainWindow::setupUi() {
  // Main layout
  // Use AdwOverlaySplitView for sidebar

  // Check if AdwOverlaySplitView is available (Adwaita 1.4+)
  // If compiling against older, might need fallback. Assuming 1.4+.
#if ADW_CHECK_VERSION(1, 4, 0)
  m_splitView = adw_overlay_split_view_new();
  adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window),
                                     m_splitView);

  // Sidebar
  m_sidebar = std::make_unique<Sidebar>();
  adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(m_splitView),
                                     m_sidebar->getWidget());

  // Connect navigation signal
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
      // Folder selection
      std::string folderId = page.substr(7);
      m_folderView->loadFolder(folderId);
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_contentStack),
                                            "folder");
    }
    // Handle other pages
  });

  // Content
  m_contentStack = adw_view_stack_new();
  adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(m_splitView),
                                     m_contentStack);

  // Library View
  m_libraryView = std::make_unique<LibraryView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_libraryView->getWidget(), "library");

  // Monitors View
  m_monitorsView = std::make_unique<MonitorsView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_monitorsView->getWidget(), "monitors");

  // Folder View
  m_folderView = std::make_unique<FolderView>();
  adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
                           m_folderView->getWidget(), "folder");

#else
  // Fallback for older libadwaita
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window), box);

  m_sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
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
