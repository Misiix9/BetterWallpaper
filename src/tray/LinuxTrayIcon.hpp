#pragma once
#include "ITrayIcon.hpp"
#include "../core/ipc/LinuxIPCClient.hpp"
#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <string>

namespace bwp::tray {

class LinuxTrayIcon : public ITrayIcon {
public:
  LinuxTrayIcon();
  ~LinuxTrayIcon() override;

  void run() override;

private:
  void setupMenu();
  void refreshMenuState();
  bool ensureConnected();

  // Static callbacks
  static void onMenuShow(GtkWidget *widget, gpointer user_data);
  static void onNext(GtkMenuItem *item, gpointer user_data);
  static void onPrevious(GtkMenuItem *item, gpointer user_data);
  static void onPause(GtkMenuItem *item, gpointer user_data);
  static void onQuit(GtkMenuItem *item, gpointer user_data);
  static void onShow(GtkMenuItem *item, gpointer user_data);
  static void onSettings(GtkMenuItem *item, gpointer user_data);
  static void onVolumeUp(GtkMenuItem *item, gpointer user_data);
  static void onVolumeDown(GtkMenuItem *item, gpointer user_data);
  static void onMute(GtkMenuItem *item, gpointer user_data);

  AppIndicator *m_indicator = nullptr;
  GtkWidget *m_menu = nullptr;

  // Dynamic menu items (stored for show/hide and label updates)
  GtkWidget *m_currentLabel = nullptr;
  GtkWidget *m_nextItem = nullptr;
  GtkWidget *m_prevItem = nullptr;
  GtkWidget *m_pauseItem = nullptr;
  GtkWidget *m_muteItem = nullptr;
  GtkWidget *m_volUpItem = nullptr;
  GtkWidget *m_volDownItem = nullptr;
  GtkWidget *m_audioSep = nullptr;

  bwp::ipc::LinuxIPCClient m_client;
  bool m_connected = false;

  // Cached state from last status query
  int m_cachedVolume = 50;
  bool m_updatingMute = false; // guard against signal recursion
};

} // namespace bwp::tray
