#include "LinuxTrayIcon.hpp"
#include "../core/utils/Logger.hpp"
#include "../core/utils/ProcessUtils.hpp"
#include <iostream>

namespace bwp::tray {

LinuxTrayIcon::LinuxTrayIcon() {
  m_indicator = app_indicator_new(
      "betterwallpaper-tray",
      "preferences-desktop-wallpaper", // Standard icon name, should exist
      APP_INDICATOR_CATEGORY_APPLICATION_STATUS);

  app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_ACTIVE);
  app_indicator_set_title(m_indicator, "BetterWallpaper");

  setupMenu();

  if (m_client.connect()) {
    LOG_INFO("Tray connected to daemon.");
  } else {
    LOG_WARN("Tray failed to connect to daemon.");
  }
}

LinuxTrayIcon::~LinuxTrayIcon() {
  // Cleanup if needed
}

void LinuxTrayIcon::setupMenu() {
  m_menu = gtk_menu_new();

  // Open GUI
  GtkWidget *openItem = gtk_menu_item_new_with_label("Open BetterWallpaper");
  g_signal_connect(openItem, "activate", G_CALLBACK(onShow), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), openItem);

  // Separator
  GtkWidget *sep1 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep1);

  // Wallpaper controls
  GtkWidget *nextItem = gtk_menu_item_new_with_label("Next Wallpaper");
  g_signal_connect(nextItem, "activate", G_CALLBACK(onNext), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), nextItem);

  GtkWidget *prevItem = gtk_menu_item_new_with_label("Previous Wallpaper");
  g_signal_connect(prevItem, "activate", G_CALLBACK(onPrevious), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), prevItem);

  GtkWidget *pauseItem = gtk_menu_item_new_with_label("Pause/Resume");
  g_signal_connect(pauseItem, "activate", G_CALLBACK(onPause), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), pauseItem);

  // Separator
  GtkWidget *sep2 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep2);

  // Settings
  GtkWidget *settingsItem = gtk_menu_item_new_with_label("Settings");
  g_signal_connect(settingsItem, "activate", G_CALLBACK(onSettings), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), settingsItem);

  // Separator
  // Volume controls (Submenu or inline? Inline simpler for now)
  GtkWidget *muteItem = gtk_check_menu_item_new_with_label("Mute"); // Checkbox? or just "Toggle Mute"
  // If we want state, we need to query state. For now, simple action "Mute/Unmute".  
  // Let's use "Mute" toggle.
  g_signal_connect(muteItem, "activate", G_CALLBACK(onMute), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), muteItem);

  // Volume Up/Down
  GtkWidget *volUpItem = gtk_menu_item_new_with_label("Volume Up");
  g_signal_connect(volUpItem, "activate", G_CALLBACK(onVolumeUp), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), volUpItem);

  GtkWidget *volDownItem = gtk_menu_item_new_with_label("Volume Down");
  g_signal_connect(volDownItem, "activate", G_CALLBACK(onVolumeDown), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), volDownItem);

  // Separator
  GtkWidget *sep3 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep3);

  // Settings

  // Quit
  GtkWidget *quitItem = gtk_menu_item_new_with_label("Quit");
  g_signal_connect(quitItem, "activate", G_CALLBACK(onQuit), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), quitItem);

  gtk_widget_show_all(m_menu);
  app_indicator_set_menu(m_indicator, GTK_MENU(m_menu));
}

void LinuxTrayIcon::onNext(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  LOG_INFO("Tray: Next Wallpaper requested");
  self->m_client.nextWallpaper("eDP-1"); // TODO: Use actual monitor name
}

void LinuxTrayIcon::onPrevious(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  LOG_INFO("Tray: Previous Wallpaper requested");
  self->m_client.previousWallpaper("eDP-1"); // TODO: Use actual monitor name
}

void LinuxTrayIcon::onPause(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<LinuxTrayIcon *>(user_data);
  LOG_INFO("Tray: Pause requested");
  self->m_client.pauseWallpaper("eDP-1");
}

void LinuxTrayIcon::onShow(GtkMenuItem *, gpointer user_data) {
  // Launch GUI
  // system("betterwallpaper &");
  // Use ProcessUtils instead (Phase 5 fix)
  utils::ProcessUtils::runAsync("betterwallpaper");
}

void LinuxTrayIcon::onSettings(GtkMenuItem *, gpointer user_data) {
  // Launch GUI and navigate to settings
  utils::ProcessUtils::runAsync("betterwallpaper --settings");
}

void LinuxTrayIcon::onVolumeUp(GtkMenuItem *, gpointer user_data) {
    auto *self = static_cast<LinuxTrayIcon *>(user_data);
    LOG_INFO("Tray: Volume Up requested");
}

void LinuxTrayIcon::onVolumeDown(GtkMenuItem *, gpointer user_data) {
    auto *self = static_cast<LinuxTrayIcon *>(user_data);
    LOG_INFO("Tray: Volume Down requested");
}

void LinuxTrayIcon::onMute(GtkMenuItem *item, gpointer user_data) {
    auto *self = static_cast<LinuxTrayIcon *>(user_data);
    bool active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
    LOG_INFO("Tray: Mute toggled: " + std::string(active ? "True" : "False"));
    self->m_client.setMuted("eDP-1", active);
}

void LinuxTrayIcon::onQuit(GtkMenuItem *, gpointer user_data) { gtk_main_quit(); }

void LinuxTrayIcon::run() { gtk_main(); }

} // namespace bwp::tray
