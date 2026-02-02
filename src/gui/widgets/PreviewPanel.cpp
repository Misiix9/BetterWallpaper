#include "PreviewPanel.hpp"
#include "../../core/ipc/DBusClient.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/slideshow/SlideshowManager.hpp"
#include "../../core/wallpaper/WallpaperManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/NativeWallpaperSetter.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "../dialogs/ErrorDialog.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <iostream>

namespace bwp::gui {

// Fixed dimensions
// User requested 300px exactly
static constexpr int PANEL_WIDTH = 300;
static constexpr int PREVIEW_WIDTH =
    260; // Slightly smaller to fit with padding
static constexpr int PREVIEW_HEIGHT = 146;

PreviewPanel::PreviewPanel() {
  bwp::monitor::MonitorManager::getInstance().initialize();
  setupUi();
}

PreviewPanel::~PreviewPanel() {}

void PreviewPanel::setupUi() {
  // Main container with fixed width
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_size_request(m_box, PANEL_WIDTH, -1);
  gtk_widget_set_hexpand(m_box, FALSE);
  gtk_widget_set_vexpand(m_box, TRUE);
  gtk_widget_set_margin_start(m_box, 12);
  gtk_widget_set_margin_end(m_box, 12);
  gtk_widget_set_margin_top(m_box, 12);
  gtk_widget_set_margin_bottom(m_box, 12);
  gtk_widget_set_valign(m_box, GTK_ALIGN_START);

  // Preview Image
  GtkWidget *imageFrame = gtk_frame_new(nullptr);
  gtk_widget_add_css_class(imageFrame, "preview-frame");
  gtk_widget_set_size_request(imageFrame, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_halign(imageFrame, GTK_ALIGN_CENTER);

  m_imageStack = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(m_imageStack),
                                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration(GTK_STACK(m_imageStack), 300);
  gtk_widget_set_size_request(m_imageStack, PREVIEW_WIDTH, PREVIEW_HEIGHT);

  m_picture1 = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_picture1), GTK_CONTENT_FIT_COVER);
  gtk_stack_add_named(GTK_STACK(m_imageStack), m_picture1, "page1");

  m_picture2 = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_picture2), GTK_CONTENT_FIT_COVER);
  gtk_stack_add_named(GTK_STACK(m_imageStack), m_picture2, "page2");

  gtk_frame_set_child(GTK_FRAME(imageFrame), m_imageStack);
  gtk_box_append(GTK_BOX(m_box), imageFrame);

  // Title
  m_titleLabel = gtk_label_new("Select a wallpaper");
  gtk_widget_add_css_class(m_titleLabel, "title-4");
  gtk_label_set_wrap(GTK_LABEL(m_titleLabel), TRUE);
  gtk_label_set_max_width_chars(GTK_LABEL(m_titleLabel), 26);
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_top(m_titleLabel, 12);
  gtk_box_append(GTK_BOX(m_box), m_titleLabel);

  // Details
  m_detailsLabel = gtk_label_new("");
  gtk_widget_add_css_class(m_detailsLabel, "dim-label");
  gtk_widget_set_halign(m_detailsLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_box), m_detailsLabel);

  // Monitor selector
  GtkWidget *monitorBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_top(monitorBox, 12);

  GtkWidget *monitorLabel = gtk_label_new("Monitor:");
  gtk_box_append(GTK_BOX(monitorBox), monitorLabel);

  m_monitorDropdown = gtk_drop_down_new(nullptr, nullptr);
  gtk_widget_set_hexpand(m_monitorDropdown, TRUE);
  gtk_box_append(GTK_BOX(monitorBox), m_monitorDropdown);
  gtk_box_append(GTK_BOX(m_box), monitorBox);

  updateMonitorList();

  // Settings Expander
  GtkWidget *expander = gtk_expander_new("Wallpaper Settings");
  gtk_widget_set_margin_top(expander, 12);
  gtk_box_append(GTK_BOX(m_box), expander);

  GtkWidget *settingsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_top(settingsBox, 8);
  gtk_widget_set_margin_bottom(settingsBox, 8);
  gtk_expander_set_child(GTK_EXPANDER(expander), settingsBox);

  // Silent
  m_silentCheck = gtk_check_button_new_with_label("Silent (Mute)");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_silentCheck), TRUE);
  gtk_box_append(GTK_BOX(settingsBox), m_silentCheck);

  // No Audio Processing
  m_noAudioProcCheck =
      gtk_check_button_new_with_label("Disable Audio Processing");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_noAudioProcCheck), TRUE);
  gtk_box_append(GTK_BOX(settingsBox), m_noAudioProcCheck);

  // Disable Mouse
  m_disableMouseCheck =
      gtk_check_button_new_with_label("Disable Mouse Interaction");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_disableMouseCheck), TRUE);
  gtk_box_append(GTK_BOX(settingsBox), m_disableMouseCheck);

  // No Automute
  m_noAutomuteCheck = gtk_check_button_new_with_label("No Auto-Mute");
  gtk_box_append(GTK_BOX(settingsBox), m_noAutomuteCheck);

  // FPS
  GtkWidget *fpsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(fpsBox), gtk_label_new("FPS Limit:"));
  m_fpsSpin = gtk_spin_button_new_with_range(1, 144, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_fpsSpin), 30);
  gtk_box_append(GTK_BOX(fpsBox), m_fpsSpin);
  gtk_box_append(GTK_BOX(settingsBox), fpsBox);

  // Volume
  GtkWidget *volLabel = gtk_label_new("Volume:");
  gtk_widget_set_halign(volLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(settingsBox), volLabel);

  m_volumeScale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
  gtk_range_set_value(GTK_RANGE(m_volumeScale), 50);
  gtk_widget_set_size_request(m_volumeScale, 100, -1);
  gtk_box_append(GTK_BOX(settingsBox), m_volumeScale);

  // Scaling
  GtkWidget *scalingBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(scalingBox), gtk_label_new("Scaling:"));

  const char *scaling_options[] = {"Default", "Stretch", "Fit", "Fill",
                                   nullptr};
  m_scalingDropdown = gtk_drop_down_new_from_strings(scaling_options);
  gtk_box_append(GTK_BOX(scalingBox), m_scalingDropdown);
  gtk_box_append(GTK_BOX(settingsBox), scalingBox);

  // Apply Button
  m_applyButton = gtk_button_new_with_label("Set as Wallpaper");
  gtk_widget_add_css_class(m_applyButton, "suggested-action");
  gtk_widget_set_sensitive(m_applyButton, FALSE);
  gtk_widget_set_margin_top(m_applyButton, 12);
  g_signal_connect(m_applyButton, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     PreviewPanel *self = static_cast<PreviewPanel *>(data);
                     self->onApplyClicked();
                   }),
                   this);
  gtk_box_append(GTK_BOX(m_box), m_applyButton);

  // Rating and Fav
  GtkWidget *metaBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(metaBox, GTK_ALIGN_FILL);

  setupRating();
  gtk_box_append(GTK_BOX(metaBox), m_ratingBox);

  gtk_box_append(GTK_BOX(m_box), metaBox);

  // Status
  m_statusLabel = gtk_label_new("");
  gtk_widget_add_css_class(m_statusLabel, "dim-label");
  gtk_widget_set_halign(m_statusLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_top(m_statusLabel, 4);
  gtk_box_append(GTK_BOX(m_box), m_statusLabel);
}

void PreviewPanel::updateMonitorList() {
  auto &mm = bwp::monitor::MonitorManager::getInstance();
  auto monitors = mm.getMonitors();

  m_monitorNames.clear();

  GtkStringList *list = gtk_string_list_new(nullptr);

  if (monitors.empty()) {
    gtk_string_list_append(list, "eDP-1");
    m_monitorNames.push_back("eDP-1");
  } else {
    for (const auto &mon : monitors) {
      gtk_string_list_append(list, mon.name.c_str());
      m_monitorNames.push_back(mon.name);
    }
  }

  gtk_drop_down_set_model(GTK_DROP_DOWN(m_monitorDropdown), G_LIST_MODEL(list));
  g_object_unref(list);
}

void PreviewPanel::setWallpaper(const bwp::wallpaper::WallpaperInfo &info) {
  m_currentInfo = info;

  std::string name = std::filesystem::path(info.path).stem().string();
  gtk_label_set_text(GTK_LABEL(m_titleLabel), name.c_str());

  std::string details = "Type: ";
  switch (info.type) {
  case bwp::wallpaper::WallpaperType::StaticImage:
    details += "Image";
    break;
  case bwp::wallpaper::WallpaperType::Video:
    details += "Video";
    break;
  case bwp::wallpaper::WallpaperType::WEScene:
    details += "WE Scene";
    break;
  case bwp::wallpaper::WallpaperType::WEVideo:
    details += "WE Video";
    break;
  default:
    details += "Unknown";
  }

  if (info.workshop_id.has_value()) {
    details += "\nID: " + std::to_string(info.workshop_id.value());
  }

  gtk_label_set_text(GTK_LABEL(m_detailsLabel), details.c_str());

  loadThumbnail(info.path);
  updateRatingDisplay();
  gtk_widget_set_sensitive(m_applyButton, TRUE);
  gtk_label_set_text(GTK_LABEL(m_statusLabel), "");
}

void PreviewPanel::loadThumbnail(const std::string &path) {
  if (!std::filesystem::exists(path))
    return;
  std::string ext = std::filesystem::path(path).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  bool isImage = (ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
                  ext == ".gif" || ext == ".bmp" || ext == ".webp");
  std::string thumbnailPath;
  if (!isImage) {
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    for (const auto &previewName :
         {"preview.jpg", "preview.png", "preview.gif"}) {
      std::filesystem::path preview = dir / previewName;
      if (std::filesystem::exists(preview)) {
        thumbnailPath = preview.string();
        break;
      }
    }
    if (thumbnailPath.empty())
      return;
  } else {
    thumbnailPath = path;
  }
  GError *err = nullptr;
  GdkTexture *texture =
      gdk_texture_new_from_filename(thumbnailPath.c_str(), &err);
  if (texture) {
    const char *visible =
        gtk_stack_get_visible_child_name(GTK_STACK(m_imageStack));
    GtkWidget *target =
        (std::string(visible) == "page1") ? m_picture2 : m_picture1;
    const char *targetName =
        (std::string(visible) == "page1") ? "page2" : "page1";

    gtk_picture_set_paintable(GTK_PICTURE(target), GDK_PAINTABLE(texture));
    gtk_stack_set_visible_child_name(GTK_STACK(m_imageStack), targetName);

    g_object_unref(texture);
  }
  if (err)
    g_error_free(err);
}

void PreviewPanel::clear() {
  gtk_label_set_text(GTK_LABEL(m_titleLabel), "Select a wallpaper");
  gtk_label_set_text(GTK_LABEL(m_detailsLabel), "");
  gtk_picture_set_paintable(GTK_PICTURE(m_picture1), nullptr);
  gtk_picture_set_paintable(GTK_PICTURE(m_picture2), nullptr);
  gtk_widget_set_sensitive(m_applyButton, FALSE);
  gtk_label_set_text(GTK_LABEL(m_statusLabel), "");
}

std::string PreviewPanel::getSettingsFlags() {
  std::string flags;

  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_silentCheck)))
    flags += " --silent";

  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_noAudioProcCheck)))
    flags += " --no-audio-processing";

  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_disableMouseCheck)))
    flags += " --disable-mouse";

  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_noAutomuteCheck)))
    flags += " --noautomute";

  // FPS
  int fps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_fpsSpin));
  if (fps != 60) { // Assume 60 is default, only add if different? Or always add
    flags += " --fps " + std::to_string(fps);
  }

  // Volume
  int vol = (int)gtk_range_get_value(GTK_RANGE(m_volumeScale));
  flags += " --volume " + std::to_string(vol);

  // Scaling
  guint scaling = gtk_drop_down_get_selected(GTK_DROP_DOWN(m_scalingDropdown));
  if (scaling == 1)
    flags += " --scaling stretch";
  else if (scaling == 2)
    flags += " --scaling fit";
  else if (scaling == 3)
    flags += " --scaling fill";

  return flags;
}

// Internal helper removed
/*
bool PreviewPanel::setWallpaperWithTool(const std::string &path, ...) {
  // Removed in favor of WallpaperManager
}
*/

void PreviewPanel::onApplyClicked() {
  LOG_INFO("Apply button clicked!");

  if (m_currentInfo.path.empty()) {
    LOG_ERROR("Cannot set wallpaper: path is empty");
    gtk_label_set_text(GTK_LABEL(m_statusLabel), "✗ No wallpaper selected");
    return;
  }
  
  auto& wm = bwp::wallpaper::WallpaperManager::getInstance();
  
  // 1. Configure settings
  // Mute
  bool muted = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_silentCheck));
  wm.setMuted(muted);
  
  // FPS
  int fps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_fpsSpin));
  if (fps > 0) wm.setFpsLimit(fps);
  
  // Monitor
  guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(m_monitorDropdown));
  std::string monitor = "eDP-1";
  if (selected < m_monitorNames.size()) {
    monitor = m_monitorNames[selected];
  }
  
  // Scaling
  guint scalingIdx = gtk_drop_down_get_selected(GTK_DROP_DOWN(m_scalingDropdown));
  // Map index to ScalingMode enum (assuming order matches: Default, Stretch, Fit, Fill)
  // 0: Default (Fill?), 1: Stretch, 2: Fit, 3: Fill
  // Enum: 0: Stretch, 1: Fit, 2: Fill, 3: Center, 4: Tile, 5: Zoom
  int scalingMode = 2; // Default Fill
  switch (scalingIdx) {
    case 1: scalingMode = 0; break; // Stretch
    case 2: scalingMode = 1; break; // Fit
    case 3: scalingMode = 2; break; // Fill
  }
  wm.setScalingMode(monitor, scalingMode);

  LOG_INFO("Setting wallpaper via Manager: " + m_currentInfo.path + " on " + monitor);
  
  gtk_label_set_text(GTK_LABEL(m_statusLabel), "Setting wallpaper...");
  while (g_main_context_iteration(nullptr, FALSE)) {}

  if (wm.setWallpaper(monitor, m_currentInfo.path)) {
    gtk_label_set_text(GTK_LABEL(m_statusLabel), "✓ Wallpaper set!");
  } else {
    gtk_label_set_text(GTK_LABEL(m_statusLabel), "✗ Failed to set");
    
    GtkRoot *root = gtk_widget_get_root(GTK_WIDGET(m_applyButton));
    if (root && GTK_IS_WINDOW(root)) {
      ErrorDialog::show(GTK_WINDOW(root), "Failed to set wallpaper",
                        "Could not apply " + m_currentInfo.path);
    }
  }
}

void PreviewPanel::setupRating() {
  m_ratingBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(m_ratingBox, "linked");

  for (int i = 1; i <= 5; ++i) {
    GtkWidget *btn = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(btn), "non-starred-symbolic");
    gtk_widget_add_css_class(btn, "flat");

    // Store rating value in button
    g_object_set_data(G_OBJECT(btn), "rating_val", GINT_TO_POINTER(i));

    g_signal_connect(
        btn, "clicked", G_CALLBACK(+[](GtkButton *b, gpointer data) {
          PreviewPanel *self = static_cast<PreviewPanel *>(data);
          int r = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "rating_val"));
          self->setRating(r);
        }),
        this);

    gtk_box_append(GTK_BOX(m_ratingBox), btn);
    m_ratingButtons.push_back(btn);
  }
}

void PreviewPanel::setRating(int rating) {
  if (m_currentInfo.id.empty())
    return;

  if (m_currentInfo.rating == rating) {
    m_currentInfo.rating = 0;
  } else {
    m_currentInfo.rating = rating;
  }

  updateRatingDisplay();

  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  lib.updateWallpaper(m_currentInfo);
}

void PreviewPanel::updateRatingDisplay() {
  for (int i = 0; i < 5; ++i) {
    int starValue = i + 1;
    GtkWidget *btn = m_ratingButtons[i];
    if (m_currentInfo.rating >= starValue) {
      gtk_button_set_icon_name(GTK_BUTTON(btn), "starred-symbolic");
      gtk_widget_add_css_class(btn, "suggested-action");
    } else {
      gtk_button_set_icon_name(GTK_BUTTON(btn), "non-starred-symbolic");
      gtk_widget_remove_css_class(btn, "suggested-action");
    }
  }
}

} // namespace bwp::gui
