#pragma once
#include "ipc/DBusClient.hpp"
#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <string>

namespace bwp::tray {

class TrayIcon {
public:
  TrayIcon();
  ~TrayIcon();

  void run();

private:
  void setupMenu();
  void updateStatus();

  // Callbacks
  static void onNext(GtkMenuItem *item, gpointer user_data);
  static void onPrevious(GtkMenuItem *item, gpointer user_data);
  static void onPause(GtkMenuItem *item, gpointer user_data);
  static void onQuit(GtkMenuItem *item, gpointer user_data);
  static void onShow(GtkMenuItem *item, gpointer user_data);
  static void onSettings(GtkMenuItem *item, gpointer user_data);

  AppIndicator *m_indicator;
  GtkWidget *m_menu;
  bwp::ipc::DBusClient m_client;
};

} // namespace bwp::tray
