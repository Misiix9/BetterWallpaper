#include "LinuxTrayIcon.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/utils/SafeProcess.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace bwp::tray {

LinuxTrayIcon::LinuxTrayIcon() {
  m_indicator = app_indicator_new(
      "betterwallpaper-tray", "preferences-desktop-wallpaper",
      APP_INDICATOR_CATEGORY_APPLICATION_STATUS);

  app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_ACTIVE);
  app_indicator_set_title(m_indicator, "BetterWallpaper");

  setupMenu();

  m_connected = m_client.connect();
  if (m_connected) {
    LOG_INFO("Tray connected to daemon.");
    refreshMenuState();
  } else {
    LOG_WARN("Tray failed to connect to daemon.");
  }
}

LinuxTrayIcon::~LinuxTrayIcon() {
  if (m_menu) {
    gtk_widget_destroy(m_menu);
    m_menu = nullptr;
  }
  if (m_indicator) {
    g_object_unref(m_indicator);
    m_indicator = nullptr;
  }
}

bool LinuxTrayIcon::ensureConnected() {
  if (m_connected) {
    return true;
  }
  m_connected = m_client.connect();
  if (m_connected) {
    LOG_INFO("Tray reconnected to daemon.");
  }
  return m_connected;
}

void LinuxTrayIcon::setupMenu() {
  m_menu = gtk_menu_new();

  // Connect to "show" signal for dynamic refresh
  g_signal_connect(m_menu, "show", G_CALLBACK(onMenuShow), this);

  // --- Current wallpaper info label (non-clickable) ---
  m_currentLabel = gtk_menu_item_new_with_label("Wallpaper: (unknown)");
  gtk_widget_set_sensitive(m_currentLabel, FALSE);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_currentLabel);

  GtkWidget *sep0 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep0);

  // --- Open / Settings ---
  GtkWidget *openItem = gtk_menu_item_new_with_label("Open BetterWallpaper");
  g_signal_connect(openItem, "activate", G_CALLBACK(onShow), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), openItem);

  GtkWidget *settingsItem = gtk_menu_item_new_with_label("Settings");
  g_signal_connect(settingsItem, "activate", G_CALLBACK(onSettings), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), settingsItem);

  GtkWidget *sep1 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep1);

  // --- Playback controls (conditionally visible) ---
  m_nextItem = gtk_menu_item_new_with_label("Next Wallpaper");
  g_signal_connect(m_nextItem, "activate", G_CALLBACK(onNext), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_nextItem);

  m_prevItem = gtk_menu_item_new_with_label("Previous Wallpaper");
  g_signal_connect(m_prevItem, "activate", G_CALLBACK(onPrevious), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_prevItem);

  m_pauseItem = gtk_menu_item_new_with_label("Pause");
  g_signal_connect(m_pauseItem, "activate", G_CALLBACK(onPause), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_pauseItem);

  // --- Audio controls (conditionally visible) ---
  m_audioSep = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_audioSep);

  m_muteItem = gtk_check_menu_item_new_with_label("Mute");
  g_signal_connect(m_muteItem, "activate", G_CALLBACK(onMute), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_muteItem);

  m_volUpItem = gtk_menu_item_new_with_label("Volume Up");
  g_signal_connect(m_volUpItem, "activate", G_CALLBACK(onVolumeUp), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_volUpItem);

  m_volDownItem = gtk_menu_item_new_with_label("Volume Down");
  g_signal_connect(m_volDownItem, "activate", G_CALLBACK(onVolumeDown), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_volDownItem);

  // --- Quit ---
  GtkWidget *sep3 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep3);

  GtkWidget *quitItem = gtk_menu_item_new_with_label("Quit");
  g_signal_connect(quitItem, "activate", G_CALLBACK(onQuit), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), quitItem);

  gtk_widget_show_all(m_menu);
  app_indicator_set_menu(m_indicator, GTK_MENU(m_menu));
}

void LinuxTrayIcon::refreshMenuState() {
  if (!ensureConnected()) {
    // Offline — hide dynamic items, show fallback label
    gtk_menu_item_set_label(GTK_MENU_ITEM(m_currentLabel),
                            "Daemon not running");
    gtk_widget_hide(m_nextItem);
    gtk_widget_hide(m_prevItem);
    gtk_widget_hide(m_pauseItem);
    gtk_widget_hide(m_audioSep);
    gtk_widget_hide(m_muteItem);
    gtk_widget_hide(m_volUpItem);
    gtk_widget_hide(m_volDownItem);
    return;
  }

  std::string statusJson = m_client.getStatus();
  if (statusJson.empty() || statusJson == "{}") {
    // Daemon may have gone away — mark disconnected for next attempt
    m_connected = false;
    gtk_menu_item_set_label(GTK_MENU_ITEM(m_currentLabel),
                            "Daemon not responding");
    gtk_widget_hide(m_nextItem);
    gtk_widget_hide(m_prevItem);
    gtk_widget_hide(m_pauseItem);
    gtk_widget_hide(m_audioSep);
    gtk_widget_hide(m_muteItem);
    gtk_widget_hide(m_volUpItem);
    gtk_widget_hide(m_volDownItem);
    return;
  }

  try {
    auto j = nlohmann::json::parse(statusJson);

    // --- Current wallpaper label (#36) ---
    std::string wallpaperDisplay = "No wallpaper";
    if (j.contains("monitors") && j["monitors"].is_array() &&
        !j["monitors"].empty()) {
      std::string path =
          j["monitors"][0].value("wallpaper", std::string{});
      if (!path.empty()) {
        // Show just the filename, truncated
        auto pos = path.rfind('/');
        std::string filename =
            (pos != std::string::npos) ? path.substr(pos + 1) : path;
        if (filename.size() > 35) {
          filename = filename.substr(0, 32) + "...";
        }
        wallpaperDisplay = filename;
      }
    }
    gtk_menu_item_set_label(GTK_MENU_ITEM(m_currentLabel),
                            wallpaperDisplay.c_str());

    // --- Slideshow items (#25) ---
    bool slideshowRunning = j.value("slideshow_running", false);
    int slideshowCount = j.value("slideshow_count", 0);
    bool showSlideshow = slideshowRunning && slideshowCount > 1;

    if (showSlideshow) {
      gtk_widget_show(m_nextItem);
      gtk_widget_show(m_prevItem);
    } else {
      gtk_widget_hide(m_nextItem);
      gtk_widget_hide(m_prevItem);
    }

    // --- Pause/Resume (#26) ---
    bool paused = j.value("paused", false);
    bool hasPlayback = slideshowRunning || !wallpaperDisplay.empty();

    if (hasPlayback) {
      gtk_widget_show(m_pauseItem);
      gtk_menu_item_set_label(GTK_MENU_ITEM(m_pauseItem),
                              paused ? "Resume" : "Pause");
    } else {
      gtk_widget_hide(m_pauseItem);
    }

    // --- Audio controls (#27, #35) ---
    // Show audio controls when any wallpaper is set (video/WE may have audio)
    bool hasWallpaper = (wallpaperDisplay != "No wallpaper");
    if (hasWallpaper) {
      gtk_widget_show(m_audioSep);
      gtk_widget_show(m_muteItem);
      gtk_widget_show(m_volUpItem);
      gtk_widget_show(m_volDownItem);

      // Sync mute checkbox (#35) — guard against recursive signal
      bool muted = j.value("muted", false);
      m_updatingMute = true;
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_muteItem),
                                     muted ? TRUE : FALSE);
      m_updatingMute = false;

      // Cache volume for Up/Down (#28)
      m_cachedVolume = j.value("volume", 50);
    } else {
      gtk_widget_hide(m_audioSep);
      gtk_widget_hide(m_muteItem);
      gtk_widget_hide(m_volUpItem);
      gtk_widget_hide(m_volDownItem);
    }

  } catch (const nlohmann::json::exception &e) {
    LOG_ERROR("Tray: Failed to parse daemon status: " + std::string(e.what()));
    gtk_menu_item_set_label(GTK_MENU_ITEM(m_currentLabel),
                            "Status error");
  }
}

// --- Static callbacks ---

void LinuxTrayIcon::onMenuShow(GtkWidget *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  self->refreshMenuState();
}

void LinuxTrayIcon::onNext(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  if (!self->ensureConnected()) return;
  LOG_INFO("Tray: Next Wallpaper requested");
  self->m_client.nextWallpaper("");
}

void LinuxTrayIcon::onPrevious(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  if (!self->ensureConnected()) return;
  LOG_INFO("Tray: Previous Wallpaper requested");
  self->m_client.previousWallpaper("");
}

void LinuxTrayIcon::onPause(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  if (!self->ensureConnected()) return;
  // Read the current label to decide action
  const gchar *label =
      gtk_menu_item_get_label(GTK_MENU_ITEM(self->m_pauseItem));
  if (label && std::string(label) == "Resume") {
    LOG_INFO("Tray: Resume requested");
    self->m_client.resumeWallpaper("");
  } else {
    LOG_INFO("Tray: Pause requested");
    self->m_client.pauseWallpaper("");
  }
}

void LinuxTrayIcon::onShow(GtkMenuItem *, gpointer) {
  utils::SafeProcess::execDetached({"betterwallpaper"});
}

void LinuxTrayIcon::onSettings(GtkMenuItem *, gpointer) {
  // #29: Launch GUI directly (no --settings flag)
  utils::SafeProcess::execDetached({"betterwallpaper"});
}

void LinuxTrayIcon::onVolumeUp(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  if (!self->ensureConnected()) return;
  int newVol = std::min(100, self->m_cachedVolume + 10);
  LOG_INFO("Tray: Volume Up " + std::to_string(self->m_cachedVolume) + " -> " +
           std::to_string(newVol));
  self->m_client.setVolume("", newVol);
  self->m_cachedVolume = newVol;
}

void LinuxTrayIcon::onVolumeDown(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  if (!self->ensureConnected()) return;
  int newVol = std::max(0, self->m_cachedVolume - 10);
  LOG_INFO("Tray: Volume Down " + std::to_string(self->m_cachedVolume) +
           " -> " + std::to_string(newVol));
  self->m_client.setVolume("", newVol);
  self->m_cachedVolume = newVol;
}

void LinuxTrayIcon::onMute(GtkMenuItem *item, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  // Skip if we're programmatically updating the checkbox
  if (self->m_updatingMute) return;
  if (!self->ensureConnected()) return;
  bool active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
  LOG_INFO("Tray: Mute toggled: " + std::string(active ? "True" : "False"));
  self->m_client.setMuted("", active);
}

void LinuxTrayIcon::onQuit(GtkMenuItem *, gpointer) { gtk_main_quit(); }

void LinuxTrayIcon::run() { gtk_main(); }

} // namespace bwp::tray
