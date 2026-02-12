#include "DownloadIndicator.hpp"
#include "../../core/steam/SteamAPIClient.hpp"
#include "../../core/utils/Logger.hpp"
#include <algorithm>
#include <curl/curl.h>
#include <thread>

namespace bwp::gui {

DownloadIndicator::DownloadIndicator() { setupUi(); }

DownloadIndicator::~DownloadIndicator() {
  if (m_popover && gtk_widget_get_parent(m_popover)) {
    gtk_widget_unparent(m_popover);
  }
}

void DownloadIndicator::setupUi() {
  // The overlay container — positioned bottom-right via alignment in parent.
  m_overlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign(m_overlay, GTK_ALIGN_END);
  gtk_widget_set_valign(m_overlay, GTK_ALIGN_END);
  gtk_widget_set_margin_end(m_overlay, 16);
  gtk_widget_set_margin_bottom(m_overlay, 16);
  // Always visible now
  gtk_widget_set_visible(m_overlay, TRUE);

  // Circular download icon button
  m_button = gtk_button_new();
  gtk_widget_add_css_class(m_button, "download-indicator-btn");
  gtk_widget_add_css_class(m_button, "circular");
  gtk_widget_set_tooltip_text(m_button, "Downloads");

  GtkWidget *icon = gtk_image_new_from_icon_name("folder-download-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 20);
  gtk_button_set_child(GTK_BUTTON(m_button), icon);
  gtk_box_append(GTK_BOX(m_overlay), m_button);

  // ── Popover ──
  m_popover = gtk_popover_new();
  gtk_widget_set_parent(m_popover, m_button);
  gtk_popover_set_position(GTK_POPOVER(m_popover), GTK_POS_TOP);
  gtk_popover_set_has_arrow(GTK_POPOVER(m_popover), TRUE);
  gtk_popover_set_autohide(GTK_POPOVER(m_popover), TRUE);
  gtk_widget_add_css_class(m_popover, "download-manager-popover");

  // Set a minimum size for the popover content
  gtk_widget_set_size_request(m_popover, 320, -1);

  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_top(mainBox, 12);
  gtk_widget_set_margin_bottom(mainBox, 12);
  gtk_widget_set_margin_start(mainBox, 12);
  gtk_widget_set_margin_end(mainBox, 12);

  // Header
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_margin_bottom(headerBox, 12);

  GtkWidget *header = gtk_label_new("Downloads");
  gtk_widget_add_css_class(header, "title-4");
  gtk_widget_set_halign(header, GTK_ALIGN_START);
  gtk_widget_set_hexpand(header, TRUE);
  gtk_box_append(GTK_BOX(headerBox), header);

  // Clear History Button
  GtkWidget *clearBtn =
      gtk_button_new_from_icon_name("edit-clear-all-symbolic");
  gtk_widget_add_css_class(clearBtn, "flat");
  gtk_widget_set_tooltip_text(clearBtn, "Clear History");
  g_signal_connect(clearBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer d) {
                     static_cast<DownloadIndicator *>(d)->clearHistory();
                   }),
                   this);
  gtk_box_append(GTK_BOX(headerBox), clearBtn);

  gtk_box_append(GTK_BOX(mainBox), headerBox);

  // Active Downloads Section
  m_activeBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(mainBox), m_activeBox);

  // Separator
  GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_margin_top(sep, 8);
  gtk_widget_set_margin_bottom(sep, 8);
  gtk_box_append(GTK_BOX(mainBox), sep);

  // History Section
  GtkWidget *historyHeader = gtk_label_new("Finished");
  gtk_widget_add_css_class(historyHeader, "heading");
  gtk_widget_set_halign(historyHeader, GTK_ALIGN_START);
  gtk_widget_set_margin_bottom(historyHeader, 4);
  gtk_box_append(GTK_BOX(mainBox), historyHeader);

  m_historyBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(mainBox), m_historyBox);

  // Empty State
  m_emptyLabel = gtk_label_new("Nothing to see here");
  gtk_widget_add_css_class(m_emptyLabel, "dim-label");
  gtk_widget_set_halign(m_emptyLabel, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(m_emptyLabel, 20);
  gtk_widget_set_margin_bottom(m_emptyLabel, 20);
  gtk_box_append(GTK_BOX(mainBox), m_emptyLabel);

  gtk_popover_set_child(GTK_POPOVER(m_popover), mainBox);

  // Click toggles popover
  g_signal_connect(m_button, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer d) {
                     auto *self = static_cast<DownloadIndicator *>(d);
                     if (gtk_widget_get_visible(self->m_popover))
                       gtk_popover_popdown(GTK_POPOVER(self->m_popover));
                     else {
                       self->refreshPopover();
                       gtk_popover_popup(GTK_POPOVER(self->m_popover));
                     }
                   }),
                   this);

  refreshPopover();
}

void DownloadIndicator::refreshPopover() {
  // Clear existing items in boxes
  GtkWidget *child = gtk_widget_get_first_child(m_activeBox);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(m_activeBox), child);
    child = next;
  }

  child = gtk_widget_get_first_child(m_historyBox);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(m_historyBox), child);
    child = next;
  }

  bool hasActive = !m_activeDownloads.empty();
  bool hasHistory = !m_history.empty();

  auto buildRow = [&](DownloadItem &item) -> GtkWidget * {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(row, 6);

    // Thumbnail / Icon
    GtkWidget *imageWidget;
    if (item.texture) {
      imageWidget =
          gtk_picture_new_for_paintable(GDK_PAINTABLE(item.texture.get()));
      gtk_picture_set_can_shrink(GTK_PICTURE(imageWidget), TRUE);
      gtk_picture_set_content_fit(GTK_PICTURE(imageWidget),
                                  GTK_CONTENT_FIT_COVER);
      // Force small size despite content
      gtk_widget_set_size_request(imageWidget, 50, 50);
      gtk_widget_set_hexpand(imageWidget, FALSE);
      gtk_widget_set_vexpand(imageWidget, FALSE);
    } else {
      // Placeholder icon or trigger load
      if (!item.thumbnailUrl.empty() && !item.isLoading) {
        loadThumbnail(item.id, item.thumbnailUrl);
      }
      imageWidget = gtk_image_new_from_icon_name("folder-download-symbolic");
      gtk_image_set_pixel_size(GTK_IMAGE(imageWidget), 24);
      gtk_widget_set_size_request(imageWidget, 50, 50);
      gtk_widget_add_css_class(imageWidget, "frame"); // distinct background
    }
    gtk_widget_set_valign(imageWidget, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), imageWidget);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_hexpand(vbox, TRUE);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);

    // Title
    GtkWidget *title = gtk_label_new(item.title.c_str());
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(title), 20);
    gtk_widget_add_css_class(title, "heading");
    gtk_box_append(GTK_BOX(vbox), title);

    // Metadata (Author • Votes)
    if (!item.author.empty()) {
      std::string metaText = item.author;
      if (item.votesUp > 0) {
        metaText += " • " + std::to_string(item.votesUp) + " votes";
      }
      GtkWidget *meta = gtk_label_new(metaText.c_str());
      gtk_widget_set_halign(meta, GTK_ALIGN_START);
      gtk_widget_add_css_class(meta, "caption");
      gtk_widget_add_css_class(meta, "dim-label");
      gtk_box_append(GTK_BOX(vbox), meta);
    }

    // Status / Progress
    if (item.finished) {
      GtkWidget *statusBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
      GtkWidget *statusIcon = gtk_image_new_from_icon_name(
          item.failed ? "dialog-error-symbolic" : "emblem-ok-symbolic");
      if (item.failed)
        gtk_widget_add_css_class(statusIcon, "error");
      else
        gtk_widget_add_css_class(statusIcon, "success");

      GtkWidget *statusLabel =
          gtk_label_new(item.failed ? "Failed" : "Downloaded");
      gtk_widget_add_css_class(statusLabel, "caption");
      if (item.failed)
        gtk_widget_add_css_class(statusLabel, "error");

      gtk_box_append(GTK_BOX(statusBox), statusIcon);
      gtk_box_append(GTK_BOX(statusBox), statusLabel);
      gtk_box_append(GTK_BOX(vbox), statusBox);
    } else {
      // Progress Bar
      GtkWidget *progress = gtk_progress_bar_new();
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), item.progress);

      if (item.progress <= 0.0f) {
        GtkWidget *statusLabel = gtk_label_new("Queued");
        gtk_widget_add_css_class(statusLabel, "caption");
        gtk_widget_add_css_class(statusLabel, "dim-label");
        gtk_widget_set_halign(statusLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(vbox), statusLabel);
      } else {
        gtk_box_append(GTK_BOX(vbox), progress);
      }
    }

    gtk_box_append(GTK_BOX(row), vbox);

    // Retry Button (only if failed)
    if (item.failed) {
      GtkWidget *retryBtn =
          gtk_button_new_from_icon_name("view-refresh-symbolic");
      gtk_widget_add_css_class(retryBtn, "flat");
      gtk_widget_set_tooltip_text(retryBtn, "Retry Download");
      gtk_widget_set_valign(retryBtn, GTK_ALIGN_CENTER);

      std::string *idPtr = new std::string(item.id);
      g_object_set_data_full(
          G_OBJECT(retryBtn), "workshop-id", idPtr,
          [](gpointer data) { delete static_cast<std::string *>(data); });

      g_signal_connect(retryBtn, "clicked",
                       G_CALLBACK(+[](GtkButton *btn, gpointer d) {
                         auto *self = static_cast<DownloadIndicator *>(d);
                         std::string *id = static_cast<std::string *>(
                             g_object_get_data(G_OBJECT(btn), "workshop-id"));
                         if (self->m_retryCallback && id) {
                           self->m_retryCallback(*id);
                         }
                       }),
                       this);

      gtk_box_append(GTK_BOX(row), retryBtn);
    }

    return row;
  };

  // Populate Active
  for (auto &item : m_activeDownloads) {
    gtk_box_append(GTK_BOX(m_activeBox), buildRow(item));
  }

  // Populate History
  for (auto &item : m_history) {
    gtk_box_append(GTK_BOX(m_historyBox), buildRow(item));
  }

  gtk_widget_set_visible(m_activeBox, hasActive);
  gtk_widget_set_visible(m_historyBox, hasHistory);
  gtk_widget_set_visible(m_emptyLabel, !hasActive && !hasHistory);
}

void DownloadIndicator::updateProgress(const std::string &workshopId,
                                       const std::string &title,
                                       float percent) {
  auto it =
      std::find_if(m_activeDownloads.begin(), m_activeDownloads.end(),
                   [&](const DownloadItem &i) { return i.id == workshopId; });

  if (it != m_activeDownloads.end()) {
    it->progress = percent;
    if (!title.empty())
      it->title = title;
  } else {
    DownloadItem item;
    item.id = workshopId;
    item.title = title.empty() ? workshopId : title;
    item.progress = percent;
    m_activeDownloads.insert(m_activeDownloads.begin(), item);
  }

  std::string tooltip =
      "Downloads: " + std::to_string(m_activeDownloads.size()) + " active";
  gtk_widget_set_tooltip_text(m_button, tooltip.c_str());

  if (!m_activeDownloads.empty()) {
    gtk_widget_add_css_class(m_button, "accent");
  } else {
    gtk_widget_remove_css_class(m_button, "accent");
  }

  if (gtk_widget_get_visible(m_popover)) {
    refreshPopover();
  }
}

void DownloadIndicator::updateFromQueue(
    const std::vector<bwp::steam::QueueItem> &queue) {
  std::vector<DownloadItem> newActive;

  for (const auto &qItem : queue) {
    if (qItem.status == bwp::steam::QueueItem::Status::Pending ||
        qItem.status == bwp::steam::QueueItem::Status::Downloading) {

      DownloadItem item;
      item.id = qItem.workshopId;
      item.title = qItem.title;
      // Map metadata
      item.thumbnailUrl = qItem.thumbnailUrl;
      item.author = qItem.author;
      item.votesUp = qItem.votesUp;

      // Try to preserve texture/loading state from existing active list
      auto existing = std::find_if(
          m_activeDownloads.begin(), m_activeDownloads.end(),
          [&](const DownloadItem &i) { return i.id == qItem.workshopId; });

      if (existing != m_activeDownloads.end()) {
        item.texture = existing->texture;
        item.isLoading = existing->isLoading;
      }
      // Check history too
      else {
        auto existingHist = std::find_if(
            m_history.begin(), m_history.end(),
            [&](const DownloadItem &i) { return i.id == qItem.workshopId; });
        if (existingHist != m_history.end()) {
          item.texture = existingHist->texture;
        }
      }

      item.progress =
          qItem.progress.percent > 0 ? qItem.progress.percent / 100.0f : 0.0f;
      newActive.push_back(item);
    }
  }

  m_activeDownloads = newActive;

  std::string tooltip =
      "Downloads: " + std::to_string(m_activeDownloads.size()) + " active";
  gtk_widget_set_tooltip_text(m_button, tooltip.c_str());

  if (!m_activeDownloads.empty()) {
    gtk_widget_add_css_class(m_button, "accent");
  } else {
    gtk_widget_remove_css_class(m_button, "accent");
  }

  if (gtk_widget_get_visible(m_popover)) {
    refreshPopover();
  }
}

void DownloadIndicator::downloadComplete(const std::string &workshopId,
                                         bool success) {
  auto it =
      std::find_if(m_activeDownloads.begin(), m_activeDownloads.end(),
                   [&](const DownloadItem &i) { return i.id == workshopId; });

  if (it != m_activeDownloads.end()) {
    DownloadItem item = *it;
    item.finished = true;
    item.progress = 1.0f;
    item.failed = !success;

    // Texture preserved by shared_ptr copy
    m_activeDownloads.erase(it);

    addHistoryItem(item);
  } else if (!success) {
    DownloadItem item;
    item.id = workshopId;
    item.title = "Workshop Item " + workshopId;
    item.finished = true;
    item.failed = true;
    addHistoryItem(item);
  }

  if (m_activeDownloads.empty()) {
    gtk_widget_remove_css_class(m_button, "accent");
    gtk_widget_set_tooltip_text(m_button, "Downloads");
  }

  if (gtk_widget_get_visible(m_popover)) {
    refreshPopover();
  }
}

void DownloadIndicator::onRetry(RetryCallback cb) { m_retryCallback = cb; }

void DownloadIndicator::addHistoryItem(const DownloadItem &item) {
  // Remove any existing history item with same ID to avoid duplicates
  auto it =
      std::remove_if(m_history.begin(), m_history.end(),
                     [&](const DownloadItem &i) { return i.id == item.id; });
  m_history.erase(it, m_history.end());

  m_history.insert(m_history.begin(), item);

  // Limit history size
  if (m_history.size() > 10) {
    m_history.pop_back();
  }
}

void DownloadIndicator::clearHistory() {
  m_history.clear();
  refreshPopover();
}

void DownloadIndicator::hide() {
  // No-op
}

void DownloadIndicator::loadThumbnail(std::string id, std::string url) {
  auto it = std::find_if(m_activeDownloads.begin(), m_activeDownloads.end(),
                         [&](const DownloadItem &i) { return i.id == id; });
  if (it != m_activeDownloads.end()) {
    it->isLoading = true;
  }

  std::thread([this, id, url]() {
    CURL *curl = curl_easy_init();
    std::string imgData;
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(
          curl, CURLOPT_WRITEFUNCTION,
          +[](void *contents, size_t size, size_t nmemb,
              void *userp) -> size_t {
            static_cast<std::string *>(userp)->append(
                static_cast<char *>(contents), size * nmemb);
            return size * nmemb;
          });
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imgData);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "BetterWallpaper/1.0");
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      CURLcode res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);

      if (res == CURLE_OK && !imgData.empty()) {
        GBytes *bytes = g_bytes_new(imgData.data(), imgData.size());
        // Marshal to main thread to create texture
        g_idle_add(
            +[](gpointer data) -> gboolean {
              auto *pkg =
                  static_cast<std::pair<DownloadIndicator *,
                                        std::pair<std::string, GBytes *>> *>(
                      data);
              auto *self = pkg->first;
              std::string itemId = pkg->second.first;
              GBytes *b = pkg->second.second;

              GInputStream *stream = g_memory_input_stream_new_from_bytes(b);
              GdkPixbuf *pixbuf =
                  gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);
              g_object_unref(stream);
              g_bytes_unref(b);

              if (pixbuf) {
                // Scale down if too large (e.g. max 100px) to prevent huge
                // natural size in UI
                int w = gdk_pixbuf_get_width(pixbuf);
                int h = gdk_pixbuf_get_height(pixbuf);
                int targetSize = 100; // Small enough but high DPI friendly

                if (w > targetSize || h > targetSize) {
                  double scale;
                  if (w < h) {
                    scale = (double)targetSize / w;
                  } else {
                    scale = (double)targetSize / h;
                  }
                  int newW = std::max(1, (int)(w * scale));
                  int newH = std::max(1, (int)(h * scale));

                  GdkPixbuf *scaled = gdk_pixbuf_scale_simple(
                      pixbuf, newW, newH, GDK_INTERP_BILINEAR);
                  g_object_unref(pixbuf);
                  pixbuf = scaled;
                }

                GdkTexture *textureRaw = gdk_texture_new_for_pixbuf(pixbuf);
                g_object_unref(pixbuf);

                std::shared_ptr<GdkTexture> texture(textureRaw,
                                                    [](GdkTexture *t) {
                                                      if (t)
                                                        g_object_unref(t);
                                                    });

                // Update item in active downloads
                auto it = std::find_if(
                    self->m_activeDownloads.begin(),
                    self->m_activeDownloads.end(),
                    [&](const DownloadItem &i) { return i.id == itemId; });
                if (it != self->m_activeDownloads.end()) {
                  it->texture = texture;
                  it->isLoading = false;
                } else {
                  // Maybe it moved to history?
                  auto hit = std::find_if(
                      self->m_history.begin(), self->m_history.end(),
                      [&](const DownloadItem &i) { return i.id == itemId; });
                  if (hit != self->m_history.end()) {
                    hit->texture = texture;
                    hit->isLoading = false;
                  }
                }

                // Refresh UI if popover visible
                if (gtk_widget_get_visible(self->m_popover)) {
                  self->refreshPopover();
                }
              }
              delete pkg;
              return FALSE;
            },
            new std::pair<DownloadIndicator *,
                          std::pair<std::string, GBytes *>>(this, {id, bytes}));
        return;
      }
    }
  }).detach();
}

} // namespace bwp::gui
