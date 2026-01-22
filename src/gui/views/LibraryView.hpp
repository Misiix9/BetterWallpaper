#pragma once
#include "../widgets/SearchBar.hpp"
#include "../widgets/WallpaperGrid.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>

namespace bwp::gui {

class LibraryView {
public:
  LibraryView();
  ~LibraryView();

  GtkWidget *getWidget() const { return m_box; }
  void refresh();

private:
  void setupUi();
  void loadWallpapers();

  GtkWidget *m_box;
  GtkWidget *m_toolbarView; // AdwToolbarView

  std::unique_ptr<SearchBar> m_searchBar;
  std::unique_ptr<WallpaperGrid> m_grid;
};

} // namespace bwp::gui
