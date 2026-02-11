#include "FavoritesView.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
namespace bwp::gui {
FavoritesView::FavoritesView() {
  setupUi();
  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<FavoritesView *>(data);
        self->loadFavorites();
        return G_SOURCE_REMOVE;
      },
      this);
  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  m_changeCallbackId = lib.addChangeCallbackWithId(
      [this](const bwp::wallpaper::WallpaperInfo &changedInfo) {
        auto *info = new bwp::wallpaper::WallpaperInfo(changedInfo);
        g_idle_add(
            +[](gpointer data) -> gboolean {
              auto *pair =
                  static_cast<std::pair<FavoritesView *,
                                        bwp::wallpaper::WallpaperInfo *> *>(
                      data);
              auto *self = pair->first;
              auto *changed = pair->second;
              if (self->m_grid) {
                if (changed->favorite) {
                  self->m_grid->updateWallpaperInStore(*changed);
                  self->m_grid->addWallpaper(*changed);
                  self->m_grid->notifyDataChanged();
                } else {
                  self->loadFavorites();
                }
              }
              delete changed;
              delete pair;
              return G_SOURCE_REMOVE;
            },
            new std::pair<FavoritesView *, bwp::wallpaper::WallpaperInfo *>(
                this, info));
      });
}
FavoritesView::~FavoritesView() {
  if (m_changeCallbackId != 0) {
    bwp::wallpaper::WallpaperLibrary::getInstance().removeChangeCallback(m_changeCallbackId);
  }
}
void FavoritesView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(header, 24);
  gtk_widget_set_margin_top(header, 24);
  gtk_widget_set_margin_bottom(header, 12);
  GtkWidget *title = gtk_label_new("Favorites");
  gtk_widget_add_css_class(title, "title-2");
  gtk_box_append(GTK_BOX(header), title);
  GtkWidget *hint =
      gtk_label_new("Click the ★ icon on any wallpaper to add it here");
  gtk_widget_add_css_class(hint, "dim-label");
  gtk_widget_set_hexpand(hint, TRUE);
  gtk_widget_set_halign(hint, GTK_ALIGN_END);
  gtk_widget_set_margin_end(hint, 24);
  gtk_box_append(GTK_BOX(header), hint);
  gtk_box_append(GTK_BOX(m_box), header);
  m_stack = gtk_stack_new();
  gtk_widget_set_vexpand(m_stack, TRUE);
  gtk_box_append(GTK_BOX(m_box), m_stack);
  m_grid = std::make_unique<WallpaperGrid>();
  gtk_stack_add_named(GTK_STACK(m_stack), m_grid->getWidget(), "grid");
  m_emptyState = adw_status_page_new();
  adw_status_page_set_icon_name(ADW_STATUS_PAGE(m_emptyState),
                                "starred-symbolic");
  adw_status_page_set_title(ADW_STATUS_PAGE(m_emptyState), "No Favorites Yet");
  adw_status_page_set_description(
      ADW_STATUS_PAGE(m_emptyState),
      "Star wallpapers in your Library to add them to Favorites");
  GtkWidget *goToLibBtn = gtk_button_new_with_label("Browse Library");
  gtk_widget_add_css_class(goToLibBtn, "suggested-action");
  gtk_widget_add_css_class(goToLibBtn, "pill");
  gtk_widget_set_halign(goToLibBtn, GTK_ALIGN_CENTER);
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
  if (favoriteCount == 0) {
    gtk_stack_set_visible_child_name(GTK_STACK(m_stack), "empty");
  } else {
    gtk_stack_set_visible_child_name(GTK_STACK(m_stack), "grid");
  }
}
}  
