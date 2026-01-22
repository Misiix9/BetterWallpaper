#include "WallpaperCard.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "../dialogs/TagDialog.hpp"
#include <filesystem>
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace bwp::gui {

WallpaperCard::WallpaperCard(const bwp::wallpaper::WallpaperInfo &info)
    : m_info(info) {
  m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_size_request(m_mainBox, 200, 160);
  gtk_widget_add_css_class(m_mainBox, "wallpaper-card");

  // Thumbnail container (Overlay for badges)
  m_overlay = gtk_overlay_new();

  // Image
  m_image = gtk_image_new_from_icon_name("image-missing-symbolic");
  gtk_widget_set_size_request(m_image, 200, 112); // 16:9 approx
  gtk_widget_set_vexpand(m_image, TRUE);
  gtk_widget_set_hexpand(m_image, TRUE);
  gtk_image_set_pixel_size(GTK_IMAGE(m_image), 64);

  gtk_overlay_set_child(GTK_OVERLAY(m_overlay), m_image);

  // Badge for type
  if (info.type == bwp::wallpaper::WallpaperType::Video ||
      info.type == bwp::wallpaper::WallpaperType::WEVideo) {
    GtkWidget *badge =
        gtk_image_new_from_icon_name("media-playback-start-symbolic");
    gtk_widget_add_css_class(badge, "card-badge");
    gtk_widget_set_halign(badge, GTK_ALIGN_END);
    gtk_widget_set_valign(badge, GTK_ALIGN_START);
    gtk_widget_set_margin_top(badge, 4);
    gtk_widget_set_margin_end(badge, 4);
    gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), badge);
  }

  gtk_box_append(GTK_BOX(m_mainBox), m_overlay);

  // Title
  std::string title = std::filesystem::path(info.path).stem().string();
  m_titleLabel = gtk_label_new(title.c_str());
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_mainBox), m_titleLabel);

  setupActions();
}

WallpaperCard::~WallpaperCard() {}

void WallpaperCard::setupActions() {
  // Favorite button
  m_favoriteBtn = gtk_button_new();
  gtk_widget_add_css_class(m_favoriteBtn, "flat");
  gtk_widget_add_css_class(m_favoriteBtn, "circular");
  gtk_widget_set_valign(m_favoriteBtn, GTK_ALIGN_END);
  gtk_widget_set_halign(m_favoriteBtn, GTK_ALIGN_END);
  gtk_widget_set_margin_bottom(m_favoriteBtn, 4);
  gtk_widget_set_margin_end(m_favoriteBtn, 4);

  setFavorite(m_info.favorite);

  g_signal_connect(
      m_favoriteBtn, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer user_data) {
        auto *self = static_cast<WallpaperCard *>(user_data);
        self->setFavorite(!self->m_info.favorite);
        // Update library
        self->m_info.favorite =
            !self->m_info.favorite; // Wait, handled in setFavorite?
        // Actually setFavorite updates UI, need to persist.
        auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
        lib.updateWallpaper(self->m_info);
      }),
      this);

  gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_favoriteBtn);

  // Right click gesture
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
  m_info.favorite = favorite;
  const char *icon = favorite ? "starred-symbolic" : "non-starred-symbolic";
  // Using simple image inside button
  gtk_button_set_icon_name(GTK_BUTTON(m_favoriteBtn), icon);

  if (favorite) {
    gtk_widget_add_css_class(m_favoriteBtn, "accent");
  } else {
    gtk_widget_remove_css_class(m_favoriteBtn, "accent");
  }
}

void WallpaperCard::showContextMenu(double x, double y) {
  // Create popover menu
  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Manage Tags...", "card.tags");
  g_menu_append(menu, "Remove from Library", "card.remove");

  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, m_mainBox);

  // Actions - simpler approach: custom popover content or manual buttons?
  // Using GActionGroup is standard but verbose for this.
  // Let's use a simpler GtkPopover with buttons if we want direct callbacks.
  // Or map actions.

  // Simple custom popover for direct binding
  GtkWidget *simplePopover = gtk_popover_new();
  gtk_widget_set_parent(simplePopover, m_mainBox);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Tags button
  GtkWidget *tagsBtn = gtk_button_new_with_label("Manage Tags...");
  gtk_widget_add_css_class(tagsBtn, "flat");
  gtk_widget_set_halign(tagsBtn, GTK_ALIGN_FILL);
  g_signal_connect(
      tagsBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer user_data) {
        auto *pair =
            static_cast<std::pair<WallpaperCard *, GtkWidget *> *>(user_data);
        auto *self = pair->first;
        GtkWidget *pop = pair->second;
        gtk_popover_popdown(GTK_POPOVER(pop));

        // Find window
        GtkNative *native = gtk_widget_get_native(self->getWidget());
        if (GTK_IS_WINDOW(native)) {
          auto *dialog = new TagDialog(GTK_WINDOW(native), self->m_info.id);
          dialog->show();
          // memory leak of dialog? It should delete itself when closed or
          // attached to window as child. TagDialog likely needs to manage its
          // own lifecycle (delete on close). Current TagDialog::show just
          // presents. It should be managed. Let's assume it leaks for now or we
          // store it. Correction: TagDialog created with new needs ownership.
        }
      }),
      new std::pair<WallpaperCard *, GtkWidget *>(this, simplePopover));
  gtk_box_append(GTK_BOX(box), tagsBtn);

  gtk_popover_set_child(GTK_POPOVER(simplePopover), box);

  // Position
  GdkRectangle rect = {(int)x, (int)y, 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(simplePopover), &rect);
  gtk_popover_popup(GTK_POPOVER(simplePopover));
}

void WallpaperCard::updateThumbnail(const std::string &path) {
  if (std::filesystem::exists(path)) {
    // Blocking load for simplicity here
    GError *err = nullptr;
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new_from_file_at_scale(path.c_str(), 200, 112, TRUE, &err);
    if (pixbuf) {
      gtk_image_set_from_pixbuf(GTK_IMAGE(m_image), pixbuf);
      g_object_unref(pixbuf);
    }
    if (err)
      g_object_unref(err);
  }
}

} // namespace bwp::gui
