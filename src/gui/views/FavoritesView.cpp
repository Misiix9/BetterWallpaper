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

  // Hint text
  GtkWidget *hint =
      gtk_label_new("Click the ★ icon on any wallpaper to add it here");
  gtk_widget_add_css_class(hint, "dim-label");
  gtk_widget_set_hexpand(hint, TRUE);
  gtk_widget_set_halign(hint, GTK_ALIGN_END);
  gtk_widget_set_margin_end(hint, 24);
  gtk_box_append(GTK_BOX(header), hint);

  gtk_box_append(GTK_BOX(m_box), header);

  // Stack for switching between grid and empty state
  m_stack = gtk_stack_new();
  gtk_widget_set_vexpand(m_stack, TRUE);
  gtk_box_append(GTK_BOX(m_box), m_stack);

  // Grid view
  m_grid = std::make_unique<WallpaperGrid>();
  gtk_stack_add_named(GTK_STACK(m_stack), m_grid->getWidget(), "grid");

  // Empty state
  m_emptyState = adw_status_page_new();
  adw_status_page_set_icon_name(ADW_STATUS_PAGE(m_emptyState),
                                "starred-symbolic");
  adw_status_page_set_title(ADW_STATUS_PAGE(m_emptyState), "No Favorites Yet");
  adw_status_page_set_description(
      ADW_STATUS_PAGE(m_emptyState),
      "Star wallpapers in your Library to add them to Favorites");

  // Add a button to go to library
  GtkWidget *goToLibBtn = gtk_button_new_with_label("Browse Library");
  gtk_widget_add_css_class(goToLibBtn, "suggested-action");
  gtk_widget_add_css_class(goToLibBtn, "pill");
  gtk_widget_set_halign(goToLibBtn, GTK_ALIGN_CENTER);

  // Note: Button signal would need to be hooked up by parent (MainWindow)
  // For now, just add it to status page
  adw_status_page_set_child(ADW_STATUS_PAGE(m_emptyState), goToLibBtn);
  m_goToLibraryButton = goToLibBtn;

  gtk_stack_add_named(GTK_STACK(m_stack), m_emptyState, "empty");
}

void FavoritesView::refresh() { loadFavorites(); }

void FavoritesView::setGoToLibraryCallback(std::function<void()> callback) {
  m_goToLibraryCallback = callback;
  if (m_goToLibraryButton && callback) {
    g_signal_connect(m_goToLibraryButton, "clicked",
                     G_CALLBACK(+[](GtkButton *, gpointer data) {
                       auto *self = static_cast<FavoritesView *>(data);
                       if (self->m_goToLibraryCallback) {
                         self->m_goToLibraryCallback();
                       }
                     }),
                     this);
  }
}

void FavoritesView::loadFavorites() {
  if (!m_grid)
    return;
  m_grid->clear();

  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto wallpapers = lib.getAllWallpapers();

  int favoriteCount = 0;
  for (const auto &info : wallpapers) {
    if (info.favorite) {
      m_grid->addWallpaper(info);
      favoriteCount++;
    }
  }

  // Show empty state or grid based on count
  if (favoriteCount == 0) {
    gtk_stack_set_visible_child_name(GTK_STACK(m_stack), "empty");
  } else {
    gtk_stack_set_visible_child_name(GTK_STACK(m_stack), "grid");
  }
}

} // namespace bwp::gui
