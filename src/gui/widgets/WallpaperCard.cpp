#include "WallpaperCard.hpp"
#include "../../core/wallpaper/ThumbnailCache.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "../dialogs/TagDialog.hpp"
#include <algorithm>
#include <filesystem>
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace bwp::gui {

// Card dimensions - compact square cards for 4+ columns
static constexpr int CARD_WIDTH = 160;
static constexpr int CARD_HEIGHT = 140;
static constexpr int IMAGE_HEIGHT = 110;

WallpaperCard::WallpaperCard(const bwp::wallpaper::WallpaperInfo &info)
    : m_info(info) {
  // Main box with strict fixed size
  m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(m_mainBox, CARD_WIDTH, CARD_HEIGHT);
  gtk_widget_set_hexpand(m_mainBox, FALSE);
  gtk_widget_set_vexpand(m_mainBox, FALSE);
  gtk_widget_set_halign(m_mainBox, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(m_mainBox, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class(m_mainBox, "wallpaper-card");

  // Image container - fixed size
  m_overlay = gtk_overlay_new();
  gtk_widget_set_size_request(m_overlay, CARD_WIDTH, IMAGE_HEIGHT);
  gtk_widget_set_hexpand(m_overlay, FALSE);
  gtk_widget_set_vexpand(m_overlay, FALSE);

  // Image
  m_image = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_image), GTK_CONTENT_FIT_COVER);
  gtk_widget_set_size_request(m_image, CARD_WIDTH, IMAGE_HEIGHT);
  gtk_widget_set_hexpand(m_image, FALSE);
  gtk_widget_set_vexpand(m_image, FALSE);
  gtk_widget_add_css_class(m_image, "card-image");

  gtk_overlay_set_child(GTK_OVERLAY(m_overlay), m_image);

  // Type badge
  if (info.type == bwp::wallpaper::WallpaperType::Video ||
      info.type == bwp::wallpaper::WallpaperType::WEVideo ||
      info.type == bwp::wallpaper::WallpaperType::WEScene) {
    GtkWidget *badge = gtk_image_new_from_icon_name(
        info.type == bwp::wallpaper::WallpaperType::WEScene
            ? "applications-games-symbolic"
            : "media-playback-start-symbolic");
    gtk_widget_add_css_class(badge, "card-badge");
    gtk_widget_set_halign(badge, GTK_ALIGN_END);
    gtk_widget_set_valign(badge, GTK_ALIGN_START);
    gtk_widget_set_margin_top(badge, 4);
    gtk_widget_set_margin_end(badge, 4);
    gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), badge);
  }

  gtk_box_append(GTK_BOX(m_mainBox), m_overlay);

  // Bottom bar
  GtkWidget *bottomBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_add_css_class(bottomBar, "card-bottom-bar");
  gtk_widget_set_margin_start(bottomBar, 6);
  gtk_widget_set_margin_end(bottomBar, 4);
  gtk_widget_set_margin_top(bottomBar, 4);
  gtk_widget_set_margin_bottom(bottomBar, 4);

  // Title
  std::string title = std::filesystem::path(info.path).stem().string();
  m_titleLabel = gtk_label_new(title.c_str());
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_label_set_max_width_chars(GTK_LABEL(m_titleLabel), 14);
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(m_titleLabel, TRUE);
  gtk_widget_add_css_class(m_titleLabel, "card-title");
  gtk_box_append(GTK_BOX(bottomBar), m_titleLabel);

  // Favorite button
  m_favoriteBtn = gtk_button_new();
  gtk_widget_add_css_class(m_favoriteBtn, "flat");
  gtk_widget_add_css_class(m_favoriteBtn, "circular");
  gtk_widget_add_css_class(m_favoriteBtn, "favorite-btn");
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

  gtk_box_append(GTK_BOX(bottomBar), m_favoriteBtn);
  gtk_box_append(GTK_BOX(m_mainBox), bottomBar);

  setupContextMenu();
}

WallpaperCard::~WallpaperCard() {}

void WallpaperCard::setupActions() {}

void WallpaperCard::setupContextMenu() {
  GtkGesture *rightClick = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(rightClick), 3);
  g_signal_connect(rightClick, "pressed",
                   G_CALLBACK(+[](GtkGestureClick *, int, double x, double y,
                                  gpointer user_data) {
                     auto *self = static_cast<WallpaperCard *>(user_data);
                     self->showContextMenu(x, y);
                   }),
                   this);
  gtk_widget_add_controller(m_mainBox, GTK_EVENT_CONTROLLER(rightClick));
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
  GtkWidget *popover = gtk_popover_new();
  gtk_widget_set_parent(popover, m_mainBox);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *tagsBtn = gtk_button_new_with_label("Manage Tags...");
  gtk_widget_add_css_class(tagsBtn, "flat");

  using TagCallbackData = std::pair<WallpaperCard *, GtkWidget *>;

  g_signal_connect(
      tagsBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer user_data) {
        auto *pair = static_cast<TagCallbackData *>(user_data);
        auto *self = pair->first;
        GtkWidget *pop = pair->second;
        gtk_popover_popdown(GTK_POPOVER(pop));

        GtkNative *native = gtk_widget_get_native(self->getWidget());
        if (GTK_IS_WINDOW(native)) {
          auto *dialog = new TagDialog(GTK_WINDOW(native), self->m_info.id);
          dialog->show();
        }
      }),
      new TagCallbackData(this, popover));
  gtk_box_append(GTK_BOX(box), tagsBtn);

  gtk_popover_set_child(GTK_POPOVER(popover), box);

  GdkRectangle rect = {(int)x, (int)y, 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
  gtk_popover_popup(GTK_POPOVER(popover));
}

void WallpaperCard::setInfo(const bwp::wallpaper::WallpaperInfo &info) {
  m_info = info;

  // Update Title
  std::string title = std::filesystem::path(info.path).stem().string();
  gtk_label_set_text(GTK_LABEL(m_titleLabel), title.c_str());

  // Update Favorite
  setFavorite(info.favorite);

  // Update Thumbnail
  // Use a placeholder first or clear current image to avoid flashing old image
  // But updateThumbnail handles async load, so it's fine.
  updateThumbnail(info.path);

  // Update Badge
  // Remove existing badges from overlay
  GtkWidget *child = gtk_widget_get_first_child(m_overlay);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    if (child != m_image) {
      gtk_overlay_remove_overlay(GTK_OVERLAY(m_overlay), child);
    }
    child = next;
  }

  // Add new badge if needed
  if (info.type == bwp::wallpaper::WallpaperType::Video ||
      info.type == bwp::wallpaper::WallpaperType::WEVideo ||
      info.type == bwp::wallpaper::WallpaperType::WEScene) {
    GtkWidget *badge = gtk_image_new_from_icon_name(
        info.type == bwp::wallpaper::WallpaperType::WEScene
            ? "applications-games-symbolic"
            : "media-playback-start-symbolic");
    gtk_widget_add_css_class(badge, "card-badge");
    gtk_widget_set_halign(badge, GTK_ALIGN_END);
    gtk_widget_set_valign(badge, GTK_ALIGN_START);
    gtk_widget_set_margin_top(badge, 4);
    gtk_widget_set_margin_end(badge, 4);
    gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), badge);
  }
}

void WallpaperCard::updateThumbnail(const std::string &path) {
  auto &cache = bwp::wallpaper::ThumbnailCache::getInstance();

  // Use medium size for grid cards
  auto size = bwp::wallpaper::ThumbnailCache::Size::Medium;

  // Request async load
  cache.getAsync(path, size, [this](GdkPixbuf *pixbuf) {
    if (pixbuf) {
      // Create texture from pixbuf
      GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
      if (texture) {
        gtk_picture_set_paintable(GTK_PICTURE(m_image), GDK_PAINTABLE(texture));
        g_object_unref(texture);
      }
    } else {
      // Set placeholder or keep empty
      // Could set a generic icon here if load failed
    }
  });
}

} // namespace bwp::gui
