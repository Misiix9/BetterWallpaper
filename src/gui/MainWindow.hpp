#pragma once
#include "utils/LazyView.hpp"
#include "views/FolderView.hpp"
#include "views/LibraryView.hpp"
#include "views/MonitorsView.hpp"
#include "views/WorkshopView.hpp"
#include "widgets/DownloadIndicator.hpp"
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
  void navigateTo(const std::string &page);
  GtkWindow *getGtkWindow() const { return GTK_WINDOW(m_window); }
  DownloadIndicator *getDownloadIndicator() const {
    return m_downloadIndicator.get();
  }

private:
  void setupUi();
  void loadWindowState();
  void saveWindowState();
  void ensureSettingsView();
  void ensureWorkshopView();
  void ensureFavoritesView();
  void ensureProfilesView();
  void ensureScheduleView();
  void ensureHyprlandView();
  static gboolean onCloseRequest(GtkWindow *window, gpointer user_data);
  GtkWidget *m_window;
  GtkWidget *m_toastOverlay;
  GtkWidget *m_splitView;
  GtkWidget *m_contentStack;
  std::unique_ptr<Sidebar> m_sidebar;
  std::unique_ptr<LibraryView> m_libraryView;
  std::unique_ptr<MonitorsView> m_monitorsView;
  std::unique_ptr<FolderView> m_folderView;
  LazyView<WorkshopView> m_workshopView;
  LazyView<SettingsView> m_settingsView;
  LazyView<FavoritesView> m_favoritesView;
  LazyView<ProfilesView> m_profilesView;
  LazyView<ScheduleView> m_scheduleView;
  std::unique_ptr<bwp::wallpaper::WallpaperEngineRenderer> m_weRenderer;
  bool m_userPaused = false;
  std::unique_ptr<HyprlandWorkspacesView> m_hyprlandView;
  bool m_workshopViewAdded = false;
  bool m_settingsViewAdded = false;
  bool m_favoritesViewAdded = false;
  bool m_profilesViewAdded = false;
  bool m_scheduleViewAdded = false;
  bool m_hyprlandViewAdded = false;

  std::unique_ptr<DownloadIndicator> m_downloadIndicator;
};
} // namespace bwp::gui
