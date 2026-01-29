#pragma once
#include "utils/LazyView.hpp"
#include "views/FolderView.hpp"
#include "views/LibraryView.hpp"
#include "views/MonitorsView.hpp"
#include "views/WorkshopView.hpp"
#include "widgets/Sidebar.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>

namespace bwp::gui {

class SettingsView;
class FavoritesView;
class RecentView;

class MainWindow {
public:
  MainWindow(AdwApplication *app);
  ~MainWindow();

  void show();

private:
  void setupUi();
  void loadWindowState();
  void saveWindowState();

  // Lazy view initialization helpers
  void ensureSettingsView();
  void ensureWorkshopView();
  void ensureFavoritesView();
  void ensureRecentView();

  // Callback for window close
  static gboolean onCloseRequest(GtkWindow *window, gpointer user_data);

  GtkWidget *m_window;
  GtkWidget *m_splitView; // AdwOverlaySplitView or similar
  GtkWidget *m_contentStack;

  std::unique_ptr<Sidebar> m_sidebar;

  // Core views (always loaded)
  std::unique_ptr<LibraryView> m_libraryView;
  std::unique_ptr<MonitorsView> m_monitorsView;
  std::unique_ptr<FolderView> m_folderView;

  // Lazy-loaded views (created on first access)
  LazyView<WorkshopView> m_workshopView;
  LazyView<SettingsView> m_settingsView;
  LazyView<FavoritesView> m_favoritesView;
  LazyView<RecentView> m_recentView;

  // Track if lazy views have been added to stack
  bool m_workshopViewAdded = false;
  bool m_settingsViewAdded = false;
  bool m_favoritesViewAdded = false;
  bool m_recentViewAdded = false;
};

} // namespace bwp::gui
