#include "TrayIcon.hpp"
#include "utils/Logger.hpp"
#include <iostream>

namespace bwp::tray {

TrayIcon::TrayIcon() {
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

TrayIcon::~TrayIcon() {
  // Cleanup if needed
}

void TrayIcon::setupMenu() {
  m_menu = gtk_menu_new();

  GtkWidget *showItem = gtk_menu_item_new_with_label("Settings");
  g_signal_connect(showItem, "activate", G_CALLBACK(onShow), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), showItem);

  // Separator
  GtkWidget *sep = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep);

  GtkWidget *nextItem = gtk_menu_item_new_with_label("Next Wallpaper");
  g_signal_connect(nextItem, "activate", G_CALLBACK(onNext), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), nextItem);

  GtkWidget *pauseItem = gtk_menu_item_new_with_label("Pause/Resume");
  g_signal_connect(pauseItem, "activate", G_CALLBACK(onPause), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), pauseItem);

  // Separator
  GtkWidget *sep2 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), sep2);

  GtkWidget *quitItem = gtk_menu_item_new_with_label("Quit");
  g_signal_connect(quitItem, "activate", G_CALLBACK(onQuit), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), quitItem);

  gtk_widget_show_all(m_menu);
  app_indicator_set_menu(m_indicator, GTK_MENU(m_menu));
}

void TrayIcon::onNext(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<TrayIcon *>(user_data);
  LOG_INFO("Tray: Next Wallpaper requested");
  self->m_client.nextWallpaper("eDP-1"); // TODO: Use actual monitor name
}

void TrayIcon::onPause(GtkMenuItem *, gpointer user_data) {
  auto *self = static_cast<TrayIcon *>(user_data);
  LOG_INFO("Tray: Pause requested");
  self->m_client.pauseWallpaper("eDP-1");
}

void TrayIcon::onShow(GtkMenuItem *, gpointer user_data) {
  // Launch GUI
  system("betterwallpaper &");
}

void TrayIcon::onQuit(GtkMenuItem *, gpointer user_data) { gtk_main_quit(); }

void TrayIcon::run() { gtk_main(); }

} // namespace bwp::tray
