#pragma once
#include "../../core/steam/SteamWorkshopClient.hpp"
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
  void performSearch(const std::string &query);
  void updateGrid(const std::vector<bwp::steam::WorkshopItem> &items);

  static void onSearchEnter(GtkEntry *entry, gpointer user_data);
  static void onDownloadClicked(GtkButton *button, gpointer user_data);

  GtkWidget *m_box;
  GtkWidget *m_searchBar;
  GtkWidget *m_grid;
  GtkWidget *m_scrolledWindow;
  GtkWidget *m_progressBar;

  struct ItemData {
    std::string id;
    WorkshopView *view;
  };
};

} // namespace bwp::gui
