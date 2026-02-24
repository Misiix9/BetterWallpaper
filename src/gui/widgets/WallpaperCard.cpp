#include "WallpaperCard.hpp"
#include "../../core/utils/Blurhash.hpp"
#include "../../core/utils/Constants.hpp"
#include "../../core/utils/StringUtils.hpp"
#include "../../core/wallpaper/ThumbnailCache.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <algorithm>
#include <filesystem>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <thread>
namespace bwp::gui {
static constexpr int CARD_WIDTH = 180;
static constexpr int CARD_HEIGHT = 135;
WallpaperCard::WallpaperCard(const bwp::wallpaper::WallpaperInfo &info)
    : m_info(info) {
  m_aliveToken = std::make_shared<bool>(true);
  m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(m_mainBox, TRUE);
  gtk_widget_add_css_class(m_mainBox, "wallpaper-card");
  m_overlay = gtk_overlay_new();
  gtk_widget_set_hexpand(m_overlay, TRUE);
  gtk_widget_set_vexpand(m_overlay, TRUE);
  gtk_box_append(GTK_BOX(m_mainBox), m_overlay);
  m_image = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_image), GTK_CONTENT_FIT_COVER);
  gtk_widget_set_size_request(m_image, CARD_WIDTH, CARD_HEIGHT);
  gtk_widget_set_hexpand(m_image, TRUE);
  gtk_widget_set_vexpand(m_image, FALSE);
  gtk_widget_add_css_class(m_image, "card-image");
  gtk_overlay_set_child(GTK_OVERLAY(m_overlay), m_image);
  m_skeletonOverlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(m_skeletonOverlay, "skeleton");
  gtk_widget_set_halign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_widget_set_valign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_skeletonOverlay);
  GtkWidget *titleContainer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_add_css_class(titleContainer, "card-title-overlay");
  gtk_widget_set_valign(titleContainer, GTK_ALIGN_END);
  gtk_widget_set_halign(titleContainer, GTK_ALIGN_FILL);
  std::string title = info.title;
  if (title.empty()) {
    title = std::filesystem::path(info.path).stem().string();
  }
  m_titleLabel = gtk_label_new(title.c_str());
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(m_titleLabel, TRUE);
  gtk_widget_add_css_class(m_titleLabel, "card-title-text");
  m_favoriteBtn = gtk_button_new();
  gtk_widget_add_css_class(m_favoriteBtn, "flat");
  gtk_widget_add_css_class(m_favoriteBtn, "card-fav-btn");
  gtk_widget_set_halign(m_favoriteBtn, GTK_ALIGN_END);
  gtk_widget_set_valign(m_favoriteBtn, GTK_ALIGN_CENTER);
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
  gtk_box_append(GTK_BOX(titleContainer), m_titleLabel);
  gtk_box_append(GTK_BOX(titleContainer), m_favoriteBtn);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), titleContainer);
  m_autoTagBadge = gtk_image_new_from_icon_name("weather-clear-night-symbolic");
  gtk_widget_set_tooltip_text(m_autoTagBadge, "Auto-tagged by AI");
  gtk_widget_add_css_class(m_autoTagBadge, "card-badge");
  gtk_widget_add_css_class(m_autoTagBadge, "auto-tag-badge");
  gtk_widget_set_halign(m_autoTagBadge, GTK_ALIGN_END);
  gtk_widget_set_valign(m_autoTagBadge, GTK_ALIGN_END);
  gtk_widget_set_margin_bottom(m_autoTagBadge, 8);
  gtk_widget_set_margin_end(m_autoTagBadge, 8);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_autoTagBadge);
  m_typeBadge = gtk_label_new("");
  gtk_widget_add_css_class(m_typeBadge, "card-badge");
  gtk_widget_add_css_class(m_typeBadge, "type-badge");
  gtk_widget_set_halign(m_typeBadge, GTK_ALIGN_START);
  gtk_widget_set_valign(m_typeBadge, GTK_ALIGN_START);
  gtk_widget_set_margin_top(m_typeBadge, 8);
  gtk_widget_set_margin_start(m_typeBadge, 8);
  std::string typeText;
  switch (info.type) {
  case bwp::wallpaper::WallpaperType::Video:
  case bwp::wallpaper::WallpaperType::WEVideo:
    typeText = "VIDEO";
    gtk_widget_add_css_class(m_typeBadge, "type-video");
    break;
  case bwp::wallpaper::WallpaperType::WEScene:
    typeText = "SCENE";
    gtk_widget_add_css_class(m_typeBadge, "type-scene");
    break;
  case bwp::wallpaper::WallpaperType::WEWeb:
    typeText = "WEB";
    gtk_widget_add_css_class(m_typeBadge, "type-web");
    break;
  case bwp::wallpaper::WallpaperType::AnimatedImage:
    typeText = "GIF";
    gtk_widget_add_css_class(m_typeBadge, "type-gif");
    break;
  default:
    break;
  }
  if (!typeText.empty()) {
    gtk_label_set_text(GTK_LABEL(m_typeBadge), typeText.c_str());
    gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_typeBadge);
  }
  m_scanOverlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(m_scanOverlay, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(m_scanOverlay, GTK_ALIGN_CENTER);
  GtkWidget *spinner = gtk_spinner_new();
  gtk_spinner_start(GTK_SPINNER(spinner));
  gtk_widget_set_size_request(spinner, 32, 32);
  gtk_box_append(GTK_BOX(m_scanOverlay), spinner);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_scanOverlay);
  gtk_widget_set_visible(m_autoTagBadge, info.isAutoTagged);
  gtk_widget_set_visible(m_scanOverlay, info.isScanning);
}
WallpaperCard::~WallpaperCard() {
  *m_aliveToken = false;
  if (m_thumbnailSourceId > 0) {
    g_source_remove(m_thumbnailSourceId);
    m_thumbnailSourceId = 0;
  }
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
void WallpaperCard::updateMetadata(const bwp::wallpaper::WallpaperInfo &info) {
  bool favChanged = (m_info.favorite != info.favorite);
  m_info.favorite = info.favorite;
  m_info.rating = info.rating;
  m_info.tags = info.tags;
  m_info.isAutoTagged = info.isAutoTagged;
  if (favChanged) {
    setFavorite(info.favorite);
  }
  gtk_widget_set_visible(m_autoTagBadge, info.isAutoTagged);
}
void WallpaperCard::setInfo(const bwp::wallpaper::WallpaperInfo &info) {
  m_info = info;
  std::string title = info.title;
  if (title.empty()) {
    title = std::filesystem::path(info.path).stem().string();
  }
  gtk_label_set_text(GTK_LABEL(m_titleLabel), title.c_str());
  setFavorite(info.favorite);
  gtk_picture_set_paintable(GTK_PICTURE(m_image), nullptr);
  gtk_widget_remove_css_class(m_image, "no-thumbnail");
  gtk_widget_set_visible(m_autoTagBadge, info.isAutoTagged);
  gtk_widget_set_visible(m_scanOverlay, info.isScanning);
  if (!info.blurhash.empty() && bwp::utils::blurhash::isValid(info.blurhash)) {
    constexpr int BLUR_W = 32;
    constexpr int BLUR_H = 24;
    auto pixels = bwp::utils::blurhash::decode(info.blurhash, BLUR_W, BLUR_H);
    if (!pixels.empty()) {
      GdkPixbuf *borrowed = gdk_pixbuf_new_from_data(
          pixels.data(), GDK_COLORSPACE_RGB, TRUE, 8, BLUR_W, BLUR_H,
          BLUR_W * 4, nullptr, nullptr);
      if (borrowed) {
        // Copy immediately so we don't depend on 'pixels' lifetime
        GdkPixbuf *blurPixbuf = gdk_pixbuf_copy(borrowed);
        g_object_unref(borrowed);
        if (blurPixbuf) {
          GdkPixbuf *scaled = gdk_pixbuf_scale_simple(
              blurPixbuf, CARD_WIDTH, CARD_HEIGHT, GDK_INTERP_BILINEAR);
          g_object_unref(blurPixbuf);
          if (scaled) {
            GdkTexture *texture = gdk_texture_new_for_pixbuf(scaled);
            if (texture) {
              gtk_picture_set_paintable(GTK_PICTURE(m_image),
                                        GDK_PAINTABLE(texture));
              g_object_unref(texture);
            }
            g_object_unref(scaled);
          }
        }
      }
    }
  }
  updateThumbnail(info.path);
}
void WallpaperCard::updateThumbnail(const std::string &path) {
  showSkeleton();
  if (m_thumbnailSourceId > 0) {
    g_source_remove(m_thumbnailSourceId);
    m_thumbnailSourceId = 0;
  }
  struct RequestData {
    WallpaperCard *card;
    std::string path;
  };
  RequestData *data = new RequestData{this, path};
  m_thumbnailSourceId = g_timeout_add_full(
      G_PRIORITY_DEFAULT, 80,
      [](gpointer user_data) -> gboolean {
        auto *d = static_cast<RequestData *>(user_data);
        d->card->m_thumbnailSourceId = 0;
        auto &cache = bwp::wallpaper::ThumbnailCache::getInstance();
        auto *cardPtr = d->card;
        auto aliveToken = cardPtr->m_aliveToken;
        cache.getAsync(
            d->path, bwp::wallpaper::ThumbnailCache::Size::Medium,
            [cardPtr, aliveToken, path = d->path](GdkPixbuf *pixbuf) {
              if (!*aliveToken)
                return;
              if (pixbuf) {
                GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
                if (texture) {
                  gtk_picture_set_paintable(GTK_PICTURE(cardPtr->m_image),
                                            GDK_PAINTABLE(texture));
                  g_object_unref(texture);
                }
                cardPtr->hideSkeleton();
                if (cardPtr->m_info.blurhash.empty()) {
                  auto wallpaperId = cardPtr->m_info.id;
                  auto wallpaperPath = cardPtr->m_info.path;
                  std::thread([wallpaperId, wallpaperPath]() {
                    auto &cache = bwp::wallpaper::ThumbnailCache::getInstance();
                    std::string hash = cache.computeBlurhash(
                        wallpaperPath,
                        bwp::wallpaper::ThumbnailCache::Size::Small);
                    if (!hash.empty()) {
                      auto &lib =
                          bwp::wallpaper::WallpaperLibrary::getInstance();
                      lib.updateBlurhash(wallpaperId, hash);
                    }
                  }).detach();
                }
              } else {
                cardPtr->hideSkeleton();
                gtk_picture_set_paintable(GTK_PICTURE(cardPtr->m_image),
                                          nullptr);
                gtk_widget_add_css_class(cardPtr->m_image, "no-thumbnail");
              }
            });
        return G_SOURCE_REMOVE;
      },
      data, [](gpointer data) { delete static_cast<RequestData *>(data); });
}
void WallpaperCard::cancelThumbnailLoad() {
  if (m_thumbnailSourceId > 0) {
    g_source_remove(m_thumbnailSourceId);
    m_thumbnailSourceId = 0;
  }
}
void WallpaperCard::releaseResources() {
  cancelThumbnailLoad();
  gtk_picture_set_paintable(GTK_PICTURE(m_image), nullptr);
  showSkeleton();
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
void WallpaperCard::setHighlight(const std::string &query) {
  std::string title = m_info.title;
  if (title.empty()) {
    title = std::filesystem::path(m_info.path).stem().string();
  }
  if (query.empty()) {
    gtk_label_set_text(GTK_LABEL(m_titleLabel), title.c_str());
    return;
  }
  std::string titleLower = bwp::utils::StringUtils::toLower(title);
  std::string queryLower = bwp::utils::StringUtils::toLower(query);
  size_t pos = titleLower.find(queryLower);
  if (pos == std::string::npos) {
    gtk_label_set_text(GTK_LABEL(m_titleLabel), title.c_str());
    return;
  }
  std::string before = title.substr(0, pos);
  std::string match = title.substr(pos, query.length());
  std::string after = title.substr(pos + query.length());
  char *escBefore = g_markup_escape_text(before.c_str(), -1);
  char *escMatch = g_markup_escape_text(match.c_str(), -1);
  char *escAfter = g_markup_escape_text(after.c_str(), -1);
  char *markup =
      g_strdup_printf("%s<span background='#FAD46B' color='black'>%s</span>%s",
                      escBefore, escMatch, escAfter);
  gtk_label_set_markup(GTK_LABEL(m_titleLabel), markup);
  g_free(markup);
  g_free(escAfter);
  g_free(escMatch);
  g_free(escBefore);
}
} // namespace bwp::gui
