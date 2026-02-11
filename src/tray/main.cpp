#include "TrayIconFactory.hpp"
#include "utils/Logger.hpp"
#include "TrayIconFactory.hpp"
#include "utils/Logger.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <gtk/gtk.h>
#endif
int main(int argc, char **argv) {
#ifndef _WIN32
  gtk_init(&argc, &argv);
#endif
  std::string logDir;
#ifdef _WIN32
  logDir = std::string(getenv("APPDATA")) + "/BetterWallpaper/logs";
#else
  logDir = std::string(getenv("HOME")) + "/.cache/betterwallpaper";
#endif
  bwp::utils::Logger::init(logDir);
  LOG_INFO("Starting System Tray...");
  LOG_INFO("Starting System Tray...");
  auto tray = bwp::tray::TrayIconFactory::create();
  tray->run();
  return 0;
}
