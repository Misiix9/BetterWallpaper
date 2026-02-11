#pragma once
#include "../widgets/WallpaperGrid.hpp"
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
namespace bwp::gui {
class FavoritesView {
public:
  FavoritesView();
  ~FavoritesView();
  GtkWidget *getWidget() const { return m_box; }
  void refresh();
  void setGoToLibraryCallback(std::function<void()> callback);
private:
  void setupUi();
  void loadFavorites();
  GtkWidget *m_box;
  GtkWidget *m_stack;
  GtkWidget *m_emptyState;
  GtkWidget *m_goToLibraryButton = nullptr;
  std::unique_ptr<WallpaperGrid> m_grid;
  std::function<void()> m_goToLibraryCallback;
};
}  
