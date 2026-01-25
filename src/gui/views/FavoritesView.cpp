#include "FavoritesView.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"

namespace bwp::gui {

FavoritesView::FavoritesView() {
  setupUi();
  // Defer loading to after window is shown
  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<FavoritesView *>(data);
        self->loadFavorites();
        return G_SOURCE_REMOVE;
      },
      this);
}

FavoritesView::~FavoritesView() {}

void FavoritesView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Header
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(header, 24);
  gtk_widget_set_margin_top(header, 24);
  gtk_widget_set_margin_bottom(header, 12);

  GtkWidget *title = gtk_label_new("Favorites");
  gtk_widget_add_css_class(title, "title-2");
  gtk_box_append(GTK_BOX(header), title);

  gtk_box_append(GTK_BOX(m_box), header);

  m_grid = std::make_unique<WallpaperGrid>();
  gtk_box_append(GTK_BOX(m_box), m_grid->getWidget());
}

void FavoritesView::refresh() { loadFavorites(); }

void FavoritesView::loadFavorites() {
  if (!m_grid)
    return;
  m_grid->clear();

  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto wallpapers = lib.getAllWallpapers();

  for (const auto &info : wallpapers) {
    if (info.favorite) {
      m_grid->addWallpaper(info);
    }
  }
}

} // namespace bwp::gui
