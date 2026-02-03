#include "MiniPlayerWindow.hpp"
#include "../../core/utils/Constants.hpp"
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace bwp::gui {

MiniPlayerWindow::MiniPlayerWindow() {
  setupUi();
}

MiniPlayerWindow::~MiniPlayerWindow() {
  if (m_window) {
    gtk_window_destroy(GTK_WINDOW(m_window));
  }
}

void MiniPlayerWindow::setupUi() {
  m_window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(m_window), "Mini Player");
  gtk_window_set_default_size(GTK_WINDOW(m_window), 300, 400);
  gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
  gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
  
  // Apply Liquid Glass styling to window
  gtk_widget_add_css_class(m_window, "void-bg");
  
  m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(m_mainBox, "glass-panel");
  gtk_widget_set_margin_top(m_mainBox, 10);
  gtk_widget_set_margin_bottom(m_mainBox, 10);
  gtk_widget_set_margin_start(m_mainBox, 10);
  gtk_widget_set_margin_end(m_mainBox, 10);
  gtk_window_set_child(GTK_WINDOW(m_window), m_mainBox);

  // 1. Thumbnail Area
  m_image = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_image), GTK_CONTENT_FIT_COVER);
  gtk_widget_set_size_request(m_image, 280, 158); // ~16:9
  gtk_widget_add_css_class(m_image, "card-image"); // Reuse style
  gtk_widget_set_margin_bottom(m_mainBox, 10); // spacing? No apply to child
  gtk_widget_set_margin_start(m_image, 10);
  gtk_widget_set_margin_end(m_image, 10);
  gtk_widget_set_margin_top(m_image, 10);
  gtk_box_append(GTK_BOX(m_mainBox), m_image);

  // 2. Title
  m_titleLabel = gtk_label_new("No Wallpaper Playing");
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_widget_add_css_class(m_titleLabel, "title-4");
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(m_titleLabel, 10);
  gtk_widget_set_margin_end(m_titleLabel, 10);
  gtk_widget_set_margin_bottom(m_titleLabel, 10);
  gtk_box_append(GTK_BOX(m_mainBox), m_titleLabel);

  // 3. Controls (Prev, Play, Next)
  GtkWidget *controlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_set_halign(controlsBox, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_bottom(controlsBox, 10);

  m_prevBtn = gtk_button_new_from_icon_name("media-skip-backward-symbolic");
  gtk_widget_add_css_class(m_prevBtn, "flat");
  gtk_widget_add_css_class(m_prevBtn, "circular");
  
  m_playPauseBtn = gtk_button_new_from_icon_name("media-playback-start-symbolic");
  gtk_widget_add_css_class(m_playPauseBtn, "suggested-action"); // High contrast
  gtk_widget_add_css_class(m_playPauseBtn, "circular");
  gtk_widget_set_size_request(m_playPauseBtn, 48, 48); // Big button

  m_nextBtn = gtk_button_new_from_icon_name("media-skip-forward-symbolic");
  gtk_widget_add_css_class(m_nextBtn, "flat");
  gtk_widget_add_css_class(m_nextBtn, "circular");

  gtk_box_append(GTK_BOX(controlsBox), m_prevBtn);
  gtk_box_append(GTK_BOX(controlsBox), m_playPauseBtn);
  gtk_box_append(GTK_BOX(controlsBox), m_nextBtn);
  gtk_box_append(GTK_BOX(m_mainBox), controlsBox);

  // 4. Volume
  GtkWidget *volBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_start(volBox, 20);
  gtk_widget_set_margin_end(volBox, 20);
  gtk_widget_set_margin_bottom(volBox, 20);

  GtkWidget *volIcon = gtk_image_new_from_icon_name("audio-volume-medium-symbolic");
  m_volumeScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_widget_set_hexpand(m_volumeScale, TRUE);
  
  gtk_box_append(GTK_BOX(volBox), volIcon);
  gtk_box_append(GTK_BOX(volBox), m_volumeScale);
  gtk_box_append(GTK_BOX(m_mainBox), volBox);

  // Close Logic (Hide on close)
  g_signal_connect(m_window, "close-request", G_CALLBACK(+[](GtkWindow *w, gpointer user_data) -> gboolean {
      auto *self = static_cast<MiniPlayerWindow*>(user_data);
      self->hide();
      return TRUE; // Prevent destruction
  }), this);
}

void MiniPlayerWindow::show() {
  gtk_window_present(GTK_WINDOW(m_window));
}

void MiniPlayerWindow::hide() {
  gtk_widget_set_visible(m_window, FALSE);
}

void MiniPlayerWindow::toggle() {
  if (gtk_widget_get_visible(m_window)) hide();
  else show();
}

bool MiniPlayerWindow::isVisible() const {
  return gtk_widget_get_visible(m_window);
}

void MiniPlayerWindow::setTitle(const std::string &title) {
  gtk_label_set_text(GTK_LABEL(m_titleLabel), title.c_str());
}

void MiniPlayerWindow::setPlaying(bool playing) {
  m_isPlaying = playing;
  gtk_button_set_icon_name(GTK_BUTTON(m_playPauseBtn), 
      playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic");
}

void MiniPlayerWindow::setVolume(double volume) {
    gtk_range_set_value(GTK_RANGE(m_volumeScale), volume);
}

void MiniPlayerWindow::setThumbnail(const std::string &path) {
    // Basic implementation - ideally use ThumbnailCache
    GFile *File = g_file_new_for_path(path.c_str());
    gtk_picture_set_file(GTK_PICTURE(m_image), File);
    g_object_unref(File);
}

} // namespace bwp::gui
