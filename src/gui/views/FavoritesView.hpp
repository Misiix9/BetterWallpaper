#pragma once
#include "../widgets/WallpaperGrid.hpp"
#include <gtk/gtk.h>

namespace bwp::gui {

class FavoritesView {
public:
  FavoritesView();
  ~FavoritesView();

  GtkWidget *getWidget() const { return m_box; }

  void refresh();

private:
  void setupUi();
  void loadFavorites();

  GtkWidget *m_box;
  std::unique_ptr<WallpaperGrid> m_grid;
};

} // namespace bwp::gui
