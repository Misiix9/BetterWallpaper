#include "RecentView.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <algorithm>

namespace bwp::gui {

RecentView::RecentView() {
  setupUi();
  // Defer loading to after window is shown
  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<RecentView *>(data);
        self->loadRecent();
        return G_SOURCE_REMOVE;
      },
      this);
}

RecentView::~RecentView() {}

void RecentView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Header
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(header, 24);
  gtk_widget_set_margin_top(header, 24);
  gtk_widget_set_margin_bottom(header, 12);

  GtkWidget *title = gtk_label_new("Recent");
  gtk_widget_add_css_class(title, "title-2");
  gtk_box_append(GTK_BOX(header), title);

  gtk_box_append(GTK_BOX(m_box), header);

  m_grid = std::make_unique<WallpaperGrid>();
  gtk_box_append(GTK_BOX(m_box), m_grid->getWidget());
}

void RecentView::refresh() { loadRecent(); }

void RecentView::loadRecent() {
  if (!m_grid)
    return;
  m_grid->clear();

  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto wallpapers = lib.getAllWallpapers();

  // Sort by last_used
  std::sort(
      wallpapers.begin(), wallpapers.end(),
      [](const auto &a, const auto &b) { return a.last_used > b.last_used; });

  // Limit to 50
  int count = 0;
  for (const auto &info : wallpapers) {
    if (info.last_used.time_since_epoch().count() > 0) {
      m_grid->addWallpaper(info);
      if (++count >= 50)
        break;
    }
  }
}

} // namespace bwp::gui
