#pragma once
#include "utils/LazyView.hpp"
#include "views/FolderView.hpp"
#include "views/LibraryView.hpp"
#include "views/MonitorsView.hpp"
#include "views/WorkshopView.hpp"
#include "widgets/HyprlandWorkspacesView.hpp"
#include "widgets/Sidebar.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>

namespace bwp::wallpaper {
class WallpaperEngineRenderer;
}

namespace bwp::gui {

class SettingsView;
class FavoritesView;
class ProfilesView;
class ScheduleView;

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
  void ensureProfilesView();
  void ensureScheduleView();
  void ensureHyprlandView();

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
  LazyView<ProfilesView> m_profilesView;
  LazyView<ScheduleView> m_scheduleView;

  std::unique_ptr<bwp::wallpaper::WallpaperEngineRenderer> m_weRenderer;
  bool m_userPaused = false;
  std::unique_ptr<HyprlandWorkspacesView> m_hyprlandView;

  // Track if lazy views have been added to stack
  bool m_workshopViewAdded = false;
  bool m_settingsViewAdded = false;
  bool m_favoritesViewAdded = false;
  bool m_profilesViewAdded = false;
  bool m_scheduleViewAdded = false;
  bool m_hyprlandViewAdded = false;
};

} // namespace bwp::gui
