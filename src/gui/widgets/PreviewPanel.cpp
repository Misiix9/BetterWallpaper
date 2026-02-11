#include "PreviewPanel.hpp"

#include "../../core/config/ConfigManager.hpp"
#include "../../core/ipc/LinuxIPCClient.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/slideshow/SlideshowManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/utils/ToastManager.hpp"
#include "../../core/wallpaper/NativeWallpaperSetter.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "../../core/wallpaper/WallpaperManager.hpp"
#include "../../core/wallpaper/WallpaperPreloader.hpp"
#include "../../core/wallpaper/ThumbnailCache.hpp"
#include "../dialogs/ErrorDialog.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <iostream>
#include <thread>

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
  // Main container with fixed width - Liquid Glass styling
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_size_request(m_box, PANEL_WIDTH, -1);
  gtk_widget_set_hexpand(m_box, FALSE);
  gtk_widget_set_vexpand(m_box, TRUE);
  gtk_widget_set_margin_start(m_box, 12);
  gtk_widget_set_margin_end(m_box, 12);
  gtk_widget_set_margin_top(m_box, 12);
  gtk_widget_set_margin_bottom(m_box, 12);
  gtk_widget_set_valign(m_box, GTK_ALIGN_START);
  gtk_widget_add_css_class(m_box, "preview-panel");

  // Preview Image (Always Visible at Top)
  GtkWidget *imageFrame = gtk_frame_new(nullptr);
  gtk_widget_add_css_class(imageFrame, "preview-frame");
  gtk_widget_set_size_request(imageFrame, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_halign(imageFrame, GTK_ALIGN_CENTER);

  m_imageStack = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(m_imageStack),
                                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration(GTK_STACK(m_imageStack), 300);

  // Scrolled Window for Zoom/Pan
  m_scrolledWindow = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                m_imageStack);

  setupGestures(m_scrolledWindow); // Attach to ScrolledWindow event controller

  // Initial size match
  gtk_widget_set_size_request(m_imageStack, PREVIEW_WIDTH, PREVIEW_HEIGHT);

  m_picture1 = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_picture1), GTK_CONTENT_FIT_COVER);
  gtk_stack_add_named(GTK_STACK(m_imageStack), m_picture1, "page1");

  m_picture2 = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(m_picture2), GTK_CONTENT_FIT_COVER);
  gtk_stack_add_named(GTK_STACK(m_imageStack), m_picture2, "page2");

  gtk_frame_set_child(GTK_FRAME(imageFrame), m_scrolledWindow);

  // Wrap imageFrame in an overlay for zoom indicator
  GtkWidget *previewOverlay = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(previewOverlay), imageFrame);

  // Zoom level indicator (top-right corner, initially hidden)
  m_zoomIndicator = gtk_label_new("1.0\u00d7");
  gtk_widget_add_css_class(m_zoomIndicator, "zoom-indicator");
  gtk_widget_set_halign(m_zoomIndicator, GTK_ALIGN_END);
  gtk_widget_set_valign(m_zoomIndicator, GTK_ALIGN_START);
  gtk_widget_set_margin_top(m_zoomIndicator, 8);
  gtk_widget_set_margin_end(m_zoomIndicator, 8);
  gtk_widget_set_visible(m_zoomIndicator, FALSE);
  gtk_overlay_add_overlay(GTK_OVERLAY(previewOverlay), m_zoomIndicator);

  gtk_box_append(GTK_BOX(m_box), previewOverlay);

  // Mini Video Controls (hidden by default, shown for video wallpapers)
  setupVideoControls();

  // Notebook for Controls / History
  GtkWidget *notebook = gtk_notebook_new();
  gtk_widget_set_vexpand(notebook, TRUE);
  gtk_box_append(GTK_BOX(m_box), notebook);

  // === Page 1: Apply (Controls) ===
  GtkWidget *applyPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(applyPage, 4);
  gtk_widget_set_margin_end(applyPage, 4);
  gtk_widget_set_margin_top(applyPage, 12);
  gtk_widget_set_margin_bottom(applyPage, 12);

  // Title
  m_titleLabel = gtk_label_new("Select a wallpaper");
  gtk_widget_add_css_class(m_titleLabel, "title-4");
  gtk_label_set_wrap(GTK_LABEL(m_titleLabel), TRUE);
  gtk_label_set_max_width_chars(GTK_LABEL(m_titleLabel), 26);
  gtk_label_set_ellipsize(GTK_LABEL(m_titleLabel), PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(applyPage), m_titleLabel);

  // Details
  m_detailsLabel = gtk_label_new("");
  gtk_widget_add_css_class(m_detailsLabel, "dim-label");
  gtk_widget_set_halign(m_detailsLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(applyPage), m_detailsLabel);

  // Monitor selector
  GtkWidget *monitorBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_top(monitorBox, 12);

  GtkWidget *monitorRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *monitorLabel = gtk_label_new("Monitor:");
  gtk_box_append(GTK_BOX(monitorRow), monitorLabel);

  m_monitorDropdown = gtk_drop_down_new(nullptr, nullptr);
  gtk_widget_set_hexpand(m_monitorDropdown, TRUE);
  gtk_box_append(GTK_BOX(monitorRow), m_monitorDropdown);
  gtk_box_append(GTK_BOX(monitorBox), monitorRow);
  gtk_box_append(GTK_BOX(applyPage), monitorBox);

  updateMonitorList();

  // Settings Expander
  GtkWidget *expander = gtk_expander_new("Wallpaper Settings");
  gtk_widget_set_margin_top(expander, 12);
  gtk_box_append(GTK_BOX(applyPage), expander);

  GtkWidget *settingsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_top(settingsBox, 8);
  gtk_widget_set_margin_bottom(settingsBox, 8);
  gtk_expander_set_child(GTK_EXPANDER(expander), settingsBox);

  // Silent
  m_silentCheck = gtk_check_button_new_with_label("Silent (Mute)");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_silentCheck), TRUE);
  g_signal_connect(m_silentCheck, "toggled",
                   G_CALLBACK(+[](GtkCheckButton *btn, gpointer data) {
                     auto *self = static_cast<PreviewPanel *>(data);
                     if (!self->m_currentInfo.id.empty()) {
                       self->m_currentInfo.settings.muted =
                           gtk_check_button_get_active(btn);
                       self->saveCurrentSettings();
                     }
                   }),
                   this);
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
  g_signal_connect(m_fpsSpin, "value-changed",
                   G_CALLBACK(+[](GtkSpinButton *btn, gpointer data) {
                     auto *self = static_cast<PreviewPanel *>(data);
                     if (!self->m_currentInfo.id.empty()) {
                       self->m_currentInfo.settings.fps =
                           static_cast<int>(gtk_spin_button_get_value(btn));
                       self->saveCurrentSettings();
                     }
                   }),
                   this);
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
  g_signal_connect(m_volumeScale, "value-changed",
                   G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     auto *self = static_cast<PreviewPanel *>(data);
                     if (!self->m_currentInfo.id.empty()) {
                       self->m_currentInfo.settings.volume =
                           static_cast<int>(gtk_range_get_value(range));
                       self->saveCurrentSettings();
                     }
                   }),
                   this);
  gtk_box_append(GTK_BOX(settingsBox), m_volumeScale);

  // Scaling
  GtkWidget *scalingBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(scalingBox), gtk_label_new("Scaling:"));

  const char *scaling_options[] = {"Default", "Stretch", "Fit", "Fill",
                                   nullptr};
  m_scalingDropdown = gtk_drop_down_new_from_strings(scaling_options);
  g_signal_connect(m_scalingDropdown, "notify::selected",
                   G_CALLBACK(+[](GObject *obj, GParamSpec *, gpointer data) {
                     auto *self = static_cast<PreviewPanel *>(data);
                     if (!self->m_currentInfo.id.empty()) {
                       guint sel = gtk_drop_down_get_selected(
                           GTK_DROP_DOWN(obj));
                       self->m_currentInfo.settings.scaling =
                           static_cast<bwp::wallpaper::ScalingMode>(sel);
                       self->saveCurrentSettings();
                     }
                   }),
                   this);
  gtk_box_append(GTK_BOX(scalingBox), m_scalingDropdown);
  gtk_box_append(GTK_BOX(settingsBox), scalingBox);

  // Apply Button
  m_applyButton = gtk_button_new();
  GtkWidget *applyLabel = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(applyLabel), "<span color='black' weight='bold'>Apply Wallpaper</span>");
  gtk_button_set_child(GTK_BUTTON(m_applyButton), applyLabel);
  gtk_widget_add_css_class(m_applyButton, "suggested-action");
  gtk_widget_set_sensitive(m_applyButton, FALSE);
  gtk_widget_set_margin_top(m_applyButton, 12);
  g_signal_connect(m_applyButton, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     PreviewPanel *self = static_cast<PreviewPanel *>(data);
                     self->onApplyClicked();
                   }),
                   this);
  gtk_box_append(GTK_BOX(applyPage), m_applyButton);

  // Rating and Fav
  GtkWidget *metaBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(metaBox, GTK_ALIGN_FILL);

  setupRating();
  gtk_box_append(GTK_BOX(metaBox), m_ratingBox);

  gtk_box_append(GTK_BOX(applyPage), metaBox);

  // Status
  m_statusLabel = gtk_label_new("");
  gtk_widget_add_css_class(m_statusLabel, "dim-label");
  gtk_widget_set_halign(m_statusLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_top(m_statusLabel, 4);
  gtk_box_append(GTK_BOX(applyPage), m_statusLabel);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), applyPage,
                           gtk_label_new("Start"));

  // Hide notebook tabs since we only have one page now
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
}

void PreviewPanel::updateMonitorList() {
  auto &mm = bwp::monitor::MonitorManager::getInstance();
  auto monitors = mm.getMonitors();

  m_monitorNames.clear();

  GtkStringList *list = gtk_string_list_new(nullptr);

  // Add "All Monitors" option first
  gtk_string_list_append(list, "All Monitors");

  if (monitors.empty()) {
    // If no monitors detected, maybe fallback to "eDP-1" or just rely on "All"
    // targeting eDP-1 fallback But let's keep specific names if available.
    // Actually if empty, loop below won't run.
    if (monitors.empty()) {
      // Only "All Monitors" present.
      // Maybe add eDP-1 implicit?
      // Let's assume MonitorManager always returns something if stubbed
      // correctly, or returns empty. If empty, "All Monitors" targets fallback.
    }
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

  std::string name = info.title;
  if (name.empty()) {
    name = std::filesystem::path(info.path).stem().string();
  }
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
  case bwp::wallpaper::WallpaperType::WEWeb:
    details += "WE Web";
    break;
  default:
    details += "Unknown";
  }

  // File Size
  try {
    if (std::filesystem::exists(info.path)) {
      auto fsize = std::filesystem::file_size(info.path);
      // Format to MB or KB
      char buf[64];
      if (fsize > 1024 * 1024)
        snprintf(buf, 64, "%.1f MB", fsize / 1024.0 / 1024.0);
      else
        snprintf(buf, 64, "%.1f KB", fsize / 1024.0);
      details += "\nSize: " + std::string(buf);
    }
  } catch (...) {
  }

  // Resolution (for Images)
  if (info.type == bwp::wallpaper::WallpaperType::StaticImage) {
    int w, h;
    if (gdk_pixbuf_get_file_info(info.path.c_str(), &w, &h)) {
      details += "\nRes: " + std::to_string(w) + "x" + std::to_string(h);
    }
  }

  if (info.workshop_id != 0) {
    details += "\nID: " + std::to_string(info.workshop_id);
  }

  gtk_label_set_text(GTK_LABEL(m_detailsLabel), details.c_str());

  loadThumbnail(info.path);
  updateRatingDisplay();
  gtk_widget_set_sensitive(m_applyButton, TRUE);
  gtk_label_set_text(GTK_LABEL(m_statusLabel), "");

  // Load per-wallpaper settings (fall back to global defaults when not set)
  m_updatingWidgets = true;
  auto &conf = bwp::config::ConfigManager::getInstance();

  // Muted: use per-wallpaper setting; if wallpaper has no explicit setting, use global default
  bool muted = info.settings.muted;
  if (!muted) {
    // If not explicitly muted per-wallpaper, check global default
    bool audioEnabled = conf.get<bool>("defaults.audio_enabled", false);
    muted = !audioEnabled;
  }
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_silentCheck), muted);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_noAudioProcCheck), muted);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_disableMouseCheck), TRUE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(m_noAutomuteCheck), FALSE);

  // Volume: per-wallpaper setting, fall back to global
  int volume = info.settings.volume;
  if (volume <= 0)
    volume = conf.get<int>("defaults.audio_volume", 50);
  gtk_range_set_value(GTK_RANGE(m_volumeScale), volume);

  // FPS: per-wallpaper setting (0 = use global default)
  int fpsLimit = info.settings.fps;
  if (fpsLimit <= 0)
    fpsLimit = conf.get<int>("performance.fps_limit", 60);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_fpsSpin), fpsLimit);

  // Scaling mode: per-wallpaper setting
  gtk_drop_down_set_selected(GTK_DROP_DOWN(m_scalingDropdown),
                             static_cast<guint>(info.settings.scaling));

  m_updatingWidgets = false;

  // Start preloading the wallpaper in the background for instant setting
  LOG_INFO("Starting preload for selected wallpaper: " + info.path);
  bwp::wallpaper::WallpaperPreloader::getInstance().preload(
      info.path, [](const std::string &path,
                    bwp::wallpaper::WallpaperPreloader::PreloadState state) {
        if (state == bwp::wallpaper::WallpaperPreloader::PreloadState::Ready) {
          LOG_INFO("Wallpaper preloaded and ready: " + path);
        } else if (state ==
                   bwp::wallpaper::WallpaperPreloader::PreloadState::Failed) {
          LOG_WARN("Wallpaper preload failed: " + path);
        }
      });

  // Show/hide video controls based on wallpaper type
  updateVideoControlsVisibility();
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
    return;
  }

  // 1. Save ALL settings to ConfigManager so the daemon picks them up
  //    when it creates a new renderer (daemon reads config in setWallpaper).
  auto &conf = bwp::config::ConfigManager::getInstance();

  bool muted = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_silentCheck));
  conf.set("defaults.audio_enabled", !muted);

  bool noAudioProc = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_noAudioProcCheck));
  conf.set("defaults.no_audio_processing", noAudioProc);

  bool disableMouse = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_disableMouseCheck));
  conf.set("defaults.disable_mouse", disableMouse);

  bool noAutomute = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_noAutomuteCheck));
  conf.set("defaults.no_automute", noAutomute);

  int fps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_fpsSpin));
  if (fps > 0)
    conf.set("performance.fps_limit", fps);

  int volume = static_cast<int>(gtk_range_get_value(GTK_RANGE(m_volumeScale)));
  conf.set("defaults.audio_volume", volume);

  conf.save(); // Persist to disk so the daemon can read it

  // Refresh monitor list to ensure we have latest
  updateMonitorList();

  // Monitor Selection
  // Index 0 is "All Monitors"
  // Index 1+ correspond to m_monitorNames[i-1]

  guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(m_monitorDropdown));
  bool applyAll = (selected == 0);

  std::string selectedMonitor = "eDP-1";
  if (!applyAll && !m_monitorNames.empty()) {
    // Offset by 1 because 0 is "All"
    size_t monitorIdx = selected - 1;
    if (monitorIdx < m_monitorNames.size()) {
      selectedMonitor = m_monitorNames[monitorIdx];
    } else {
      // Fallback to the first monitor if selected index is out of bounds
      selectedMonitor = m_monitorNames[0];
    }
  }

  // Set visual status immediately - REMOVED per user request
  // Only using toasts now
  gtk_label_set_text(GTK_LABEL(m_statusLabel), "");

  std::vector<std::string> targets;
  if (applyAll) {
    if (m_monitorNames.empty())
      targets.push_back("eDP-1"); // Fallback if no monitors but "All" selected?
    else
      targets = m_monitorNames;
  } else {
    targets.push_back(selectedMonitor);
  }

  LOG_INFO("Apply clicked. Target count: " + std::to_string(targets.size()));
  for (const auto &t : targets) {
    LOG_INFO("Target Monitor: " + t);
  }

  // 2. Send wallpaper command via IPC to the daemon.
  //    The daemon's WallpaperManager owns the renderers and layer-shell
  //    windows. The GUI must NOT create its own — that causes zombie
  //    processes and competing wallpaper surfaces.
  struct ApplyData {
    PreviewPanel *panel;
    std::vector<std::string> targets;
    std::string path;
  };
  ApplyData *data = new ApplyData{this, targets, m_currentInfo.path};

  // Schedule on main thread
  g_idle_add(
      +[](gpointer user_data) -> gboolean {
        ApplyData *d = static_cast<ApplyData *>(user_data);

        // Create IPC client and connect to daemon
        bwp::ipc::LinuxIPCClient ipcClient;
        bool success = true;

        if (!ipcClient.connect()) {
          LOG_ERROR("Failed to connect to daemon via IPC");
          success = false;
        } else {
          // Send SetWallpaper for each target monitor via IPC
          for (const auto &mon : d->targets) {
            LOG_INFO("Sending IPC SetWallpaper for " + mon + ": " + d->path);
            if (!ipcClient.setWallpaper(d->path, mon)) {
              LOG_ERROR("IPC SetWallpaper failed for monitor: " + mon);
              success = false;
            }
          }
        }

        // Update UI (we're already on main thread)
        if (success) {
          // Show success toast
          bwp::core::utils::ToastManager::getInstance().showSuccess(
              "Wallpaper applied successfully");
        } else {
          // Show error toast
          bwp::core::utils::ToastManager::getInstance().showError(
              "Failed to set wallpaper: " + d->path);

          GtkRoot *root =
              gtk_widget_get_root(GTK_WIDGET(d->panel->m_applyButton));
          if (root && GTK_IS_WINDOW(root)) {
            ErrorDialog::show(GTK_WINDOW(root), "Failed to set wallpaper",
                              "Could not apply " + d->path);
          }
        }

        delete d;
        return FALSE; // Run once
      },
      data);

  // Return immediately, status label already says "Setting..."
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
      gtk_widget_add_css_class(btn, "rated-star");
      gtk_widget_remove_css_class(btn, "suggested-action"); // Ensure standard class is gone
    } else {
      gtk_button_set_icon_name(GTK_BUTTON(btn), "non-starred-symbolic");
      gtk_widget_remove_css_class(btn, "rated-star");
      gtk_widget_remove_css_class(btn, "suggested-action");
    }
  }
}

void PreviewPanel::saveCurrentSettings() {
  if (m_updatingWidgets || m_currentInfo.id.empty())
    return;
  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  lib.updateWallpaper(m_currentInfo);
}

void PreviewPanel::setupGestures(GtkWidget *widget) {
  // Double-click to reset zoom
  auto *clickGesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(clickGesture),
                                GDK_BUTTON_PRIMARY);
  g_signal_connect(
      clickGesture, "pressed",
      G_CALLBACK(+[](GtkGestureClick *, int n_press, double, double,
                      gpointer data) {
        if (n_press == 2) {
          auto *self = static_cast<PreviewPanel *>(data);
          self->resetZoom();
        }
      }),
      this);
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(clickGesture));

  // Scroll wheel to zoom
  auto *scrollController = gtk_event_controller_scroll_new(
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect(
      scrollController, "scroll",
      G_CALLBACK(+[](GtkEventControllerScroll *, double, double dy,
                      gpointer data) -> gboolean {
        auto *self = static_cast<PreviewPanel *>(data);
        double newZoom = self->m_zoomLevel - dy * ZOOM_STEP;
        self->applyZoom(newZoom);
        return TRUE; // Consume the event
      }),
      this);
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(scrollController));

  // Drag to Pan
  auto *dragGesture = gtk_gesture_drag_new();
  g_signal_connect(
      dragGesture, "drag-begin",
      G_CALLBACK(+[](GtkGestureDrag *c, double x, double y, gpointer data) {
        auto *self = static_cast<PreviewPanel *>(data);
        self->onPanBegin(c, x, y);
      }),
      this);
  g_signal_connect(
      dragGesture, "drag-update",
      G_CALLBACK(+[](GtkGestureDrag *c, double x, double y, gpointer data) {
        auto *self = static_cast<PreviewPanel *>(data);
        self->onPanGesture(c, x, y);
      }),
      this);
  g_signal_connect(
      dragGesture, "drag-end",
      G_CALLBACK(+[](GtkGestureDrag *, double, double, gpointer data) {
        auto *self = static_cast<PreviewPanel *>(data);
        // Restore default cursor
        gtk_widget_set_cursor_from_name(self->m_scrolledWindow, nullptr);
      }),
      this);
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(dragGesture));
}

void PreviewPanel::applyZoom(double newZoom, double /*focusX*/,
                             double /*focusY*/) {
  newZoom = std::clamp(newZoom, ZOOM_MIN, ZOOM_MAX);
  if (std::abs(newZoom - m_zoomLevel) < 0.01)
    return;

  m_zoomLevel = newZoom;

  int w = static_cast<int>(PREVIEW_WIDTH * m_zoomLevel);
  int h = static_cast<int>(PREVIEW_HEIGHT * m_zoomLevel);
  gtk_widget_set_size_request(m_imageStack, w, h);

  updateZoomIndicator();

  // Set cursor hint based on zoom state
  if (m_zoomLevel > ZOOM_MIN) {
    gtk_widget_set_cursor_from_name(m_scrolledWindow, "grab");
  } else {
    gtk_widget_set_cursor_from_name(m_scrolledWindow, nullptr);
  }
}

void PreviewPanel::resetZoom() {
  applyZoom(ZOOM_MIN);

  // Reset scroll position to origin
  GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));
  gtk_adjustment_set_value(hadj, 0);
  gtk_adjustment_set_value(vadj, 0);
}

void PreviewPanel::updateZoomIndicator() {
  if (!m_zoomIndicator)
    return;

  if (m_zoomLevel <= ZOOM_MIN + 0.01) {
    gtk_widget_set_visible(m_zoomIndicator, FALSE);
    return;
  }

  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.1f\u00d7", m_zoomLevel);
  gtk_label_set_text(GTK_LABEL(m_zoomIndicator), buf);
  gtk_widget_set_visible(m_zoomIndicator, TRUE);

  // Auto-hide after 2 seconds
  g_timeout_add(
      2000,
      +[](gpointer data) -> gboolean {
        auto *label = static_cast<GtkWidget *>(data);
        if (GTK_IS_WIDGET(label)) {
          gtk_widget_set_visible(label, FALSE);
        }
        return G_SOURCE_REMOVE;
      },
      m_zoomIndicator);
}

void PreviewPanel::onPanBegin(GtkGestureDrag *, double /*start_x*/,
                              double /*start_y*/) {
  GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));

  m_dragStartX = gtk_adjustment_get_value(hadj);
  m_dragStartY = gtk_adjustment_get_value(vadj);

  // Grabbing cursor while dragging
  if (m_zoomLevel > ZOOM_MIN) {
    gtk_widget_set_cursor_from_name(m_scrolledWindow, "grabbing");
  }
}

void PreviewPanel::onPanGesture(GtkGestureDrag *, double offset_x,
                                double offset_y) {
  if (m_zoomLevel <= ZOOM_MIN)
    return; // No pan if not zoomed

  GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));

  gtk_adjustment_set_value(hadj, m_dragStartX - offset_x);
  gtk_adjustment_set_value(vadj, m_dragStartY - offset_y);
}

void PreviewPanel::setupVideoControls() {
  m_videoControlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_add_css_class(m_videoControlsBox, "video-controls");
  gtk_widget_set_margin_start(m_videoControlsBox, 4);
  gtk_widget_set_margin_end(m_videoControlsBox, 4);
  gtk_widget_set_margin_top(m_videoControlsBox, 6);
  gtk_widget_set_margin_bottom(m_videoControlsBox, 2);
  gtk_widget_set_visible(m_videoControlsBox, FALSE);

  // Play/Pause button
  m_playPauseBtn = gtk_button_new_from_icon_name("media-playback-start-symbolic");
  gtk_widget_add_css_class(m_playPauseBtn, "flat");
  gtk_widget_add_css_class(m_playPauseBtn, "circular");
  gtk_widget_set_tooltip_text(m_playPauseBtn, "Play / Pause");
  g_signal_connect(
      m_playPauseBtn, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<PreviewPanel *>(data);
        self->m_isVideoPlaying = !self->m_isVideoPlaying;
        gtk_button_set_icon_name(
            GTK_BUTTON(self->m_playPauseBtn),
            self->m_isVideoPlaying ? "media-playback-pause-symbolic"
                                   : "media-playback-start-symbolic");
      }),
      this);
  gtk_box_append(GTK_BOX(m_videoControlsBox), m_playPauseBtn);

  // Time label (shows type info for now since we don't have mpv preview)
  m_videoTimeLabel = gtk_label_new("Video");
  gtk_widget_add_css_class(m_videoTimeLabel, "dim-label");
  gtk_widget_add_css_class(m_videoTimeLabel, "caption");
  gtk_widget_set_hexpand(m_videoTimeLabel, TRUE);
  gtk_widget_set_halign(m_videoTimeLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_videoControlsBox), m_videoTimeLabel);

  // Volume button + scale
  m_videoVolumeBtn =
      gtk_button_new_from_icon_name("audio-volume-medium-symbolic");
  gtk_widget_add_css_class(m_videoVolumeBtn, "flat");
  gtk_widget_add_css_class(m_videoVolumeBtn, "circular");
  gtk_widget_set_tooltip_text(m_videoVolumeBtn, "Toggle Mute");
  g_signal_connect(
      m_videoVolumeBtn, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<PreviewPanel *>(data);
        double vol = gtk_range_get_value(GTK_RANGE(self->m_videoVolumeScale));
        if (vol > 0) {
          gtk_range_set_value(GTK_RANGE(self->m_videoVolumeScale), 0);
          gtk_button_set_icon_name(GTK_BUTTON(self->m_videoVolumeBtn),
                                   "audio-volume-muted-symbolic");
        } else {
          gtk_range_set_value(GTK_RANGE(self->m_videoVolumeScale), 50);
          gtk_button_set_icon_name(GTK_BUTTON(self->m_videoVolumeBtn),
                                   "audio-volume-medium-symbolic");
        }
      }),
      this);
  gtk_box_append(GTK_BOX(m_videoControlsBox), m_videoVolumeBtn);

  m_videoVolumeScale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
  gtk_range_set_value(GTK_RANGE(m_videoVolumeScale), 50);
  gtk_widget_set_size_request(m_videoVolumeScale, 60, -1);
  gtk_scale_set_draw_value(GTK_SCALE(m_videoVolumeScale), FALSE);
  g_signal_connect(
      m_videoVolumeScale, "value-changed",
      G_CALLBACK(+[](GtkRange *range, gpointer data) {
        auto *self = static_cast<PreviewPanel *>(data);
        double val = gtk_range_get_value(range);
        const char *icon = val <= 0    ? "audio-volume-muted-symbolic"
                           : val < 50  ? "audio-volume-low-symbolic"
                                       : "audio-volume-medium-symbolic";
        gtk_button_set_icon_name(GTK_BUTTON(self->m_videoVolumeBtn), icon);
      }),
      this);
  gtk_box_append(GTK_BOX(m_videoControlsBox), m_videoVolumeScale);

  gtk_box_append(GTK_BOX(m_box), m_videoControlsBox);
}

void PreviewPanel::updateVideoControlsVisibility() {
  if (!m_videoControlsBox)
    return;

  bool isVideo =
      (m_currentInfo.type == bwp::wallpaper::WallpaperType::Video ||
       m_currentInfo.type == bwp::wallpaper::WallpaperType::WEVideo ||
       m_currentInfo.type == bwp::wallpaper::WallpaperType::WEScene);

  gtk_widget_set_visible(m_videoControlsBox, isVideo);

  if (isVideo) {
    // Reset play state
    m_isVideoPlaying = false;
    gtk_button_set_icon_name(GTK_BUTTON(m_playPauseBtn),
                             "media-playback-start-symbolic");

    // Update time label with type info
    const char *typeStr = "Video";
    if (m_currentInfo.type == bwp::wallpaper::WallpaperType::WEScene)
      typeStr = "WE Scene";
    else if (m_currentInfo.type == bwp::wallpaper::WallpaperType::WEVideo)
      typeStr = "WE Video";
    gtk_label_set_text(GTK_LABEL(m_videoTimeLabel), typeStr);
  }
}

} // namespace bwp::gui

