#pragma once
#include "ITrayIcon.hpp"
#include "../core/ipc/LinuxIPCClient.hpp"
#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <string>

namespace bwp::tray {

class LinuxTrayIcon : public ITrayIcon {
public:
  LinuxTrayIcon();
  ~LinuxTrayIcon() override;

  void run() override;

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
  static void onVolumeUp(GtkMenuItem *item, gpointer user_data);
  static void onVolumeDown(GtkMenuItem *item, gpointer user_data);
  static void onMute(GtkMenuItem *item, gpointer user_data);

  AppIndicator *m_indicator;
  GtkWidget *m_menu;
  bwp::ipc::LinuxIPCClient m_client;
};

} // namespace bwp::tray
