#include "WallpaperCard.hpp"
#include "../../core/wallpaper/ThumbnailCache.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
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
  m_aliveToken = std::make_shared<bool>(true);

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

  // Skeleton overlay for loading state
  m_skeletonOverlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(m_skeletonOverlay, CARD_WIDTH, IMAGE_HEIGHT);
  gtk_widget_add_css_class(m_skeletonOverlay, "skeleton");
  gtk_widget_set_halign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_widget_set_valign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_skeletonOverlay);

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

WallpaperCard::~WallpaperCard() { *m_aliveToken = false; }

// Context menu handled by WallpaperGrid controller
void WallpaperCard::setupActions() {}

void WallpaperCard::setupContextMenu() {}

void WallpaperCard::setFavorite(bool favorite) {
  const char *icon = favorite ? "starred-symbolic" : "non-starred-symbolic";
  gtk_button_set_icon_name(GTK_BUTTON(m_favoriteBtn), icon);

  if (favorite) {
    gtk_widget_add_css_class(m_favoriteBtn, "favorited");
  } else {
    gtk_widget_remove_css_class(m_favoriteBtn, "favorited");
  }
}

void WallpaperCard::showContextMenu(double, double) {}

void WallpaperCard::setInfo(const bwp::wallpaper::WallpaperInfo &info) {
  m_info = info;

  // Update Title
  std::string title = std::filesystem::path(info.path).stem().string();
  gtk_label_set_text(GTK_LABEL(m_titleLabel), title.c_str());

  // Update Favorite
  setFavorite(info.favorite);

  // Clear old image before loading new one
  gtk_picture_set_paintable(GTK_PICTURE(m_image), nullptr);
  gtk_widget_remove_css_class(m_image, "no-thumbnail");

  // Update Badge - remove only badge overlays, not skeleton
  GtkWidget *child = gtk_widget_get_first_child(m_overlay);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    // Only remove if it's not the main image and not the skeleton overlay
    if (child != m_image && child != m_skeletonOverlay) {
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

  // Update Thumbnail (this shows/hides skeleton as needed)
  updateThumbnail(info.path);
}

void WallpaperCard::updateThumbnail(const std::string &path) {
  // Show skeleton while loading
  showSkeleton();

  auto &cache = bwp::wallpaper::ThumbnailCache::getInstance();

  // Use medium size for grid cards
  auto size = bwp::wallpaper::ThumbnailCache::Size::Medium;

  // Request async load
  // Capture alive token by value (shared ptr copy)
  cache.getAsync(path, size, [this, alive = m_aliveToken](GdkPixbuf *pixbuf) {
    if (!*alive) {
      // Card is dead, do not touch 'this' member variables
      return;
    }
    if (pixbuf) {
      // Create texture from pixbuf
      GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
      if (texture) {
        gtk_picture_set_paintable(GTK_PICTURE(m_image), GDK_PAINTABLE(texture));
        g_object_unref(texture);
      }
      // Hide skeleton after loading
      hideSkeleton();
    } else {
      // No thumbnail available - show a placeholder icon
      // Hide skeleton and show the default icon
      hideSkeleton();

      // Set a fallback icon based on content type
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
