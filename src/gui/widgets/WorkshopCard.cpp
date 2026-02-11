#include "WorkshopCard.hpp"
#include <curl/curl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <thread>

namespace bwp::gui {

static constexpr int CARD_WIDTH = 180;
static constexpr int CARD_HEIGHT = 135;

WorkshopCard::WorkshopCard(const bwp::steam::WorkshopItem& item)
    : m_item(item) {
  m_aliveToken = std::make_shared<bool>(true);

  // Main vertical box — fixed size matching library cards exactly
  m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(m_mainBox, CARD_WIDTH, CARD_HEIGHT);
  gtk_widget_set_overflow(m_mainBox, GTK_OVERFLOW_HIDDEN);
  gtk_widget_add_css_class(m_mainBox, "wallpaper-card");

  // Overlay container (same structure as library WallpaperCard)
  m_overlay = gtk_overlay_new();
  gtk_widget_set_size_request(m_overlay, CARD_WIDTH, CARD_HEIGHT);
  gtk_widget_set_overflow(m_overlay, GTK_OVERFLOW_HIDDEN);
  gtk_widget_set_hexpand(m_overlay, FALSE);
  gtk_widget_set_vexpand(m_overlay, FALSE);
  gtk_box_append(GTK_BOX(m_mainBox), m_overlay);

  // Image — fixed size, can_shrink prevents natural size from expanding parent
  m_image = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_image), GTK_CONTENT_FIT_COVER);
  gtk_picture_set_can_shrink(GTK_PICTURE(m_image), TRUE);
  gtk_widget_set_size_request(m_image, CARD_WIDTH, CARD_HEIGHT);
  gtk_widget_set_hexpand(m_image, FALSE);
  gtk_widget_set_vexpand(m_image, FALSE);
  gtk_widget_add_css_class(m_image, "card-image");
  gtk_widget_add_css_class(m_image, "no-thumbnail");
  gtk_overlay_set_child(GTK_OVERLAY(m_overlay), m_image);

  // Skeleton loading overlay
  m_skeletonOverlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(m_skeletonOverlay, "skeleton");
  gtk_widget_set_halign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_widget_set_valign(m_skeletonOverlay, GTK_ALIGN_FILL);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_skeletonOverlay);

  // Title + action button container at bottom (same layout as library)
  GtkWidget* titleContainer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_add_css_class(titleContainer, "card-title-overlay");
  gtk_widget_set_valign(titleContainer, GTK_ALIGN_END);
  gtk_widget_set_halign(titleContainer, GTK_ALIGN_FILL);

  // Title label
  std::string title = item.title;
  if (title.empty()) title = "Workshop #" + item.id;
  m_titleLabel = gtk_label_new(title.c_str());
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(m_titleLabel, TRUE);
  gtk_widget_add_css_class(m_titleLabel, "card-title-text");

  // Action button: download or checkmark (same styling as library fav btn)
  m_actionBtn = gtk_button_new_from_icon_name("folder-download-symbolic");
  gtk_widget_add_css_class(m_actionBtn, "flat");
  gtk_widget_add_css_class(m_actionBtn, "card-fav-btn"); // reuse same compact style
  gtk_widget_set_halign(m_actionBtn, GTK_ALIGN_END);
  gtk_widget_set_valign(m_actionBtn, GTK_ALIGN_CENTER);
  gtk_widget_set_tooltip_text(m_actionBtn, "Download");

  g_signal_connect(m_actionBtn, "clicked",
      G_CALLBACK(+[](GtkButton*, gpointer data) {
        auto* self = static_cast<WorkshopCard*>(data);
        if (self->m_isDownloaded) return; // Already downloaded, ignore
        if (self->m_downloadCb) {
          self->m_downloadCb(self->m_item.id, self->m_item.title);
        }
      }), this);

  gtk_box_append(GTK_BOX(titleContainer), m_titleLabel);
  gtk_box_append(GTK_BOX(titleContainer), m_actionBtn);
  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), titleContainer);

  // Load thumbnail asynchronously
  if (!item.thumbnailUrl.empty()) {
    loadThumbnailAsync(item.thumbnailUrl);
  } else {
    gtk_widget_set_visible(m_skeletonOverlay, FALSE);
  }
}

WorkshopCard::~WorkshopCard() {
  *m_aliveToken = false;
}

void WorkshopCard::setDownloaded(bool downloaded) {
  m_isDownloaded = downloaded;
  if (downloaded) {
    gtk_button_set_icon_name(GTK_BUTTON(m_actionBtn), "object-select-symbolic");
    gtk_widget_set_tooltip_text(m_actionBtn, "Already in library");
    gtk_widget_add_css_class(m_actionBtn, "success");
    gtk_widget_set_sensitive(m_actionBtn, FALSE);
  } else {
    gtk_button_set_icon_name(GTK_BUTTON(m_actionBtn), "folder-download-symbolic");
    gtk_widget_set_tooltip_text(m_actionBtn, "Download");
    gtk_widget_remove_css_class(m_actionBtn, "success");
    gtk_widget_set_sensitive(m_actionBtn, TRUE);
  }
}

void WorkshopCard::cancelThumbnailLoad() {
  *m_aliveToken = false;
  m_aliveToken = std::make_shared<bool>(true);
}

void WorkshopCard::releaseResources() {
  cancelThumbnailLoad();
  gtk_picture_set_paintable(GTK_PICTURE(m_image), nullptr);
}

void WorkshopCard::loadThumbnailAsync(const std::string& url) {
  struct ThumbData {
    GtkWidget* image;
    GtkWidget* skeleton;
    std::string url;
    std::shared_ptr<bool> alive;
  };

  auto alive = m_aliveToken;
  // Store alive token reference so it gets invalidated on widget destruction
  g_object_set_data_full(G_OBJECT(m_image), "alive-token",
      new std::shared_ptr<bool>(alive),
      [](gpointer p) { delete static_cast<std::shared_ptr<bool>*>(p); });

  auto* td = new ThumbData{m_image, m_skeletonOverlay, url, alive};
  std::thread([td]() {
    CURL* curl = curl_easy_init();
    std::string imgData;
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, td->url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
          +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
            static_cast<std::string*>(userp)->append(
                static_cast<char*>(contents), size * nmemb);
            return size * nmemb;
          });
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imgData);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "BetterWallpaper/1.0");
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      CURLcode res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      if (res != CURLE_OK) {
        delete td;
        return;
      }
    }

    GBytes* bytes = g_bytes_new(imgData.data(), imgData.size());
    GInputStream* stream = g_memory_input_stream_new_from_bytes(bytes);
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);
    g_object_unref(stream);
    g_bytes_unref(bytes);

    if (!pixbuf) {
      delete td;
      return;
    }

    // Scale pixbuf to CARD_WIDTH x CARD_HEIGHT so GtkPicture reports
    // exactly that as its natural size (prevents FlowBox stretching)
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(
        pixbuf, 180, 135, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    if (!scaled) {
      delete td;
      return;
    }

    struct IdleData {
      GdkPixbuf* pixbuf;
      GtkWidget* image;
      GtkWidget* skeleton;
      std::shared_ptr<bool> alive;
    };
    auto* id = new IdleData{scaled, td->image, td->skeleton, td->alive};
    delete td;

    g_idle_add(+[](gpointer data) -> gboolean {
      auto* d = static_cast<IdleData*>(data);
      if (*d->alive) {
        GdkTexture* texture = gdk_texture_new_for_pixbuf(d->pixbuf);
        gtk_picture_set_paintable(GTK_PICTURE(d->image), GDK_PAINTABLE(texture));
        gtk_widget_remove_css_class(d->image, "no-thumbnail");
        gtk_widget_set_visible(d->skeleton, FALSE);
        g_object_unref(texture);
      }
      g_object_unref(d->pixbuf);
      delete d;
      return G_SOURCE_REMOVE;
    }, id);
  }).detach();
}

} // namespace bwp::gui
