#include "WallpaperCard.hpp"
#include "../../core/hyprland/HyprlandManager.hpp"
#include "../../core/utils/Constants.hpp"
#include "../../core/wallpaper/ThumbnailCache.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "../dialogs/WorkspaceSelectionDialog.hpp"
#include <algorithm>
#include <filesystem>
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace bwp::gui {

// Compact Cards - 4:3 aspect ratio for better grid layout
static constexpr int CARD_WIDTH = 180;
static constexpr int CARD_HEIGHT = 135; // 180 * 3/4

WallpaperCard::WallpaperCard(const bwp::wallpaper::WallpaperInfo &info)
    : m_info(info) {
  m_aliveToken = std::make_shared<bool>(true);

  // Main container (The Card Frame)
  m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  // gtk_widget_set_size_request(m_mainBox, CARD_WIDTH, CARD_HEIGHT);
  //  Do not enforce strict size preventing expansion? Grid usually handles
  //  this. But let's keep it fixed for consistent look.
  gtk_widget_add_css_class(m_mainBox, "wallpaper-card");

  // Overlay container (Layers)
  m_overlay = gtk_overlay_new();
  gtk_widget_set_hexpand(m_overlay, TRUE);
  gtk_widget_set_vexpand(m_overlay, TRUE);
  gtk_box_append(GTK_BOX(m_mainBox), m_overlay);

  // 1. Background Image
  m_image = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_image), GTK_CONTENT_FIT_COVER);
  gtk_widget_set_size_request(m_image, CARD_WIDTH, CARD_HEIGHT);
  gtk_widget_add_css_class(m_image, "card-image");
  gtk_overlay_set_child(GTK_OVERLAY(m_overlay), m_image);

  // 2. Skeleton Overlay (Loading)
  m_skeletonOverlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(m_skeletonOverlay, "skeleton");
  gtk_widget_set_halign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_widget_set_valign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_skeletonOverlay);

  // 3. Title Overlay (Gradient + Text) - Bottom
  // Uses CSS for hover reveal
  GtkWidget *titleContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(titleContainer, "card-title-overlay");
  gtk_widget_set_valign(titleContainer, GTK_ALIGN_END);
  gtk_widget_set_halign(titleContainer, GTK_ALIGN_FILL);

  std::string title = std::filesystem::path(info.path).stem().string();
  m_titleLabel = gtk_label_new(title.c_str());
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_widget_add_css_class(m_titleLabel, "card-title-text");

  gtk_box_append(GTK_BOX(titleContainer), m_titleLabel);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), titleContainer);

  // 4. Favorite Button - Top Right
  m_favoriteBtn = gtk_button_new();
  gtk_widget_add_css_class(m_favoriteBtn, "flat");
  gtk_widget_add_css_class(m_favoriteBtn, "circular");
  gtk_widget_add_css_class(m_favoriteBtn, "card-fav-btn"); // Specific styling
  gtk_widget_set_halign(m_favoriteBtn, GTK_ALIGN_END);
  gtk_widget_set_valign(m_favoriteBtn, GTK_ALIGN_START);
  gtk_widget_set_margin_top(m_favoriteBtn, 8);
  gtk_widget_set_margin_end(m_favoriteBtn, 8);

  setFavorite(m_info.favorite);

  g_signal_connect(m_favoriteBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer user_data) {
                     auto *self = static_cast<WallpaperCard *>(user_data);
                     bool newFav = !self->m_info.favorite;
                     self->m_info.favorite = newFav;
                     self->setFavorite(newFav);
                     auto &lib =
                         bwp::wallpaper::WallpaperLibrary::getInstance();
                     lib.updateWallpaper(self->m_info);
                   }),
                   this);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_favoriteBtn);

  // Auto-Tag Badge (Sparkle) - Bottom Right
  m_autoTagBadge = gtk_image_new_from_icon_name(
      "weather-clear-night-symbolic"); // Sparkle-like
  gtk_widget_set_tooltip_text(m_autoTagBadge, "Auto-tagged by AI");
  gtk_widget_add_css_class(m_autoTagBadge, "card-badge");
  gtk_widget_add_css_class(m_autoTagBadge,
                           "auto-tag-badge"); // gold/accent color
  gtk_widget_set_halign(m_autoTagBadge, GTK_ALIGN_END);
  gtk_widget_set_valign(m_autoTagBadge, GTK_ALIGN_END);
  gtk_widget_set_margin_bottom(m_autoTagBadge, 8);
  gtk_widget_set_margin_end(m_autoTagBadge, 8);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_autoTagBadge);

  // Scan Overlay (Spinner) - Center
  m_scanOverlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(m_scanOverlay, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(m_scanOverlay, GTK_ALIGN_CENTER);
  GtkWidget *spinner = gtk_spinner_new();
  gtk_spinner_start(GTK_SPINNER(spinner));
  gtk_widget_set_size_request(spinner, 32, 32);
  gtk_box_append(GTK_BOX(m_scanOverlay), spinner);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_scanOverlay);

  // Initial State Update
  gtk_widget_set_visible(m_autoTagBadge, info.isAutoTagged);
  gtk_widget_set_visible(m_scanOverlay, info.isScanning);

  setupContextMenu();
}

WallpaperCard::~WallpaperCard() { *m_aliveToken = false; }

void WallpaperCard::setupActions() {}

void WallpaperCard::setupContextMenu() {
  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Set as Wallpaper", "card.set-wallpaper");
  g_menu_append(menu, "Set for Workspace...", "card.set-for-workspace");
  g_menu_append(menu, "Toggle Favorite", "card.toggle-favorite");

  GMenu *moreSection = g_menu_new();
  g_menu_append(moreSection, "Show in Files", "card.show-in-files");
  g_menu_append_section(menu, NULL, G_MENU_MODEL(moreSection));

  m_contextMenu = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(m_contextMenu, m_mainBox);
  gtk_popover_set_has_arrow(GTK_POPOVER(m_contextMenu), FALSE);

  GSimpleActionGroup *actions = g_simple_action_group_new();

  // Actions...
  auto createAction = [&](const char *name, auto callback) {
    GSimpleAction *act = g_simple_action_new(name, NULL);
    g_object_set_data(G_OBJECT(act), "card", this);
    g_signal_connect(act, "activate", G_CALLBACK(callback), act);
    g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(act));
  };

  createAction(
      "set-wallpaper", +[](GSimpleAction *, GVariant *, gpointer data) {
        auto *card = static_cast<WallpaperCard *>(
            g_object_get_data(G_OBJECT(data), "card"));
        if (card && card->m_setWallpaperCallback)
          card->m_setWallpaperCallback(card->m_info.path);
      });

  createAction(
      "toggle-favorite", +[](GSimpleAction *, GVariant *, gpointer data) {
        auto *card = static_cast<WallpaperCard *>(
            g_object_get_data(G_OBJECT(data), "card"));
        if (card) {
          bool newFav = !card->m_info.favorite;
          card->m_info.favorite = newFav;
          card->setFavorite(newFav);
          bwp::wallpaper::WallpaperLibrary::getInstance().updateWallpaper(
              card->m_info);
        }
      });

  createAction(
      "set-for-workspace", +[](GSimpleAction *, GVariant *, gpointer data) {
        auto *card = static_cast<WallpaperCard *>(
            g_object_get_data(G_OBJECT(data), "card"));
        if (card) {
          GtkWidget *toplevel =
              gtk_widget_get_ancestor(card->getWidget(), GTK_TYPE_WINDOW);
          if (GTK_IS_WINDOW(toplevel)) {
            auto *dialog = new WorkspaceSelectionDialog(GTK_WINDOW(toplevel),
                                                        card->getInfo().path);
            dialog->show([](const std::set<int> &) {});
          }
        }
      });

  createAction(
      "show-in-files", +[](GSimpleAction *, GVariant *, gpointer data) {
        auto *card = static_cast<WallpaperCard *>(
            g_object_get_data(G_OBJECT(data), "card"));
        if (card) {
          std::string cmd =
              "xdg-open \"" +
              std::filesystem::path(card->m_info.path).parent_path().string() +
              "\"";
          system(cmd.c_str());
        }
      });

  gtk_widget_insert_action_group(m_mainBox, "card", G_ACTION_GROUP(actions));

  GtkGesture *rightClick = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(rightClick),
                                GDK_BUTTON_SECONDARY);
  g_signal_connect(rightClick, "pressed",
                   G_CALLBACK(+[](GtkGestureClick *, gint, gdouble x, gdouble y,
                                  gpointer data) {
                     static_cast<WallpaperCard *>(data)->showContextMenu(x, y);
                   }),
                   this);
  gtk_widget_add_controller(m_mainBox, GTK_EVENT_CONTROLLER(rightClick));

  g_object_unref(menu);
  g_object_unref(moreSection);
}

void WallpaperCard::setFavorite(bool favorite) {
  const char *icon = favorite ? "starred-symbolic" : "non-starred-symbolic";
  gtk_button_set_icon_name(GTK_BUTTON(m_favoriteBtn), icon);
  if (favorite) {
    gtk_widget_add_css_class(m_favoriteBtn, "favorited");
  } else {
    gtk_widget_remove_css_class(m_favoriteBtn, "favorited");
  }
}

void WallpaperCard::showContextMenu(double x, double y) {
  if (m_contextMenu) {
    GdkRectangle rect = {(int)x, (int)y, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(m_contextMenu), &rect);
    gtk_popover_popup(GTK_POPOVER(m_contextMenu));
  }
}

void WallpaperCard::setInfo(const bwp::wallpaper::WallpaperInfo &info) {
  m_info = info;

  std::string title = std::filesystem::path(info.path).stem().string();
  gtk_label_set_text(GTK_LABEL(m_titleLabel), title.c_str());

  setFavorite(info.favorite);

  gtk_picture_set_paintable(GTK_PICTURE(m_image), nullptr);
  gtk_widget_remove_css_class(m_image, "no-thumbnail");

  // Reset Badges
  gtk_widget_set_visible(m_autoTagBadge, info.isAutoTagged);
  gtk_widget_set_visible(m_scanOverlay, info.isScanning);

  updateThumbnail(info.path);
}

void WallpaperCard::updateThumbnail(const std::string &path) {
  showSkeleton();
  auto &cache = bwp::wallpaper::ThumbnailCache::getInstance();
  cache.getAsync(path, bwp::wallpaper::ThumbnailCache::Size::Medium,
                 [this, alive = m_aliveToken](GdkPixbuf *pixbuf) {
                   if (!*alive)
                     return;
                   if (pixbuf) {
                     GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
                     if (texture) {
                       gtk_picture_set_paintable(GTK_PICTURE(m_image),
                                                 GDK_PAINTABLE(texture));
                       g_object_unref(texture);
                     }
                     hideSkeleton();
                   } else {
                     hideSkeleton();
                     gtk_picture_set_paintable(GTK_PICTURE(m_image), nullptr);
                     gtk_widget_add_css_class(m_image, "no-thumbnail");
                   }
                 });
}

void WallpaperCard::showSkeleton() {
  if (m_skeletonOverlay) {
    gtk_widget_set_visible(m_skeletonOverlay, TRUE);
    m_isLoading = true;
  }
}

void WallpaperCard::hideSkeleton() {
  if (m_skeletonOverlay) {
    gtk_widget_set_visible(m_skeletonOverlay, FALSE);
    m_isLoading = false;
  }
}

} // namespace bwp::gui
