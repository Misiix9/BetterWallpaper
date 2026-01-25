#pragma once
#include "../../core/steam/SteamWorkshopClient.hpp"
#include "../widgets/WallpaperGrid.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <vector>

namespace bwp::gui {

class WorkshopView {
public:
  WorkshopView();
  ~WorkshopView();

  GtkWidget *getWidget() { return m_box; }

private:
  void setupUi();
  void setupBrowsePage();
  void setupInstalledPage();
  void refreshInstalled();

  void performSearch(const std::string &query);
  void updateBrowseGrid(const std::vector<bwp::steam::WorkshopItem> &items);

  static void onSearchEnter(GtkEntry *entry, gpointer user_data);
  static void onDownloadClicked(GtkButton *button, gpointer user_data);

  GtkWidget *m_box;
  GtkWidget *m_stack;

  // Browse
  GtkWidget *m_browsePage;
  GtkWidget *m_searchBar;
  GtkWidget *m_progressBar;
  GtkWidget *m_browseGrid;
  GtkWidget *m_browseScrolled;

  // Installed
  GtkWidget *m_installedPage;
  std::unique_ptr<WallpaperGrid> m_installedGrid;

  struct ItemData {
    std::string id;
    WorkshopView *view;
  };
};

} // namespace bwp::gui
