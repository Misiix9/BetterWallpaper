#include "TrayIconFactory.hpp"
#include "utils/Logger.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <gtk/gtk.h>
#endif
#include <cstdlib>

int main(int argc, char **argv) {
#ifndef _WIN32
  gtk_init(&argc, &argv);
#endif

  std::string logDir;
#ifdef _WIN32
  const char *appdata = std::getenv("APPDATA");
  if (!appdata) {
    std::fprintf(stderr, "APPDATA environment variable not set\n");
    return 1;
  }
  logDir = std::string(appdata) + "/BetterWallpaper/logs";
#else
  const char *home = std::getenv("HOME");
  if (!home) {
    std::fprintf(stderr, "HOME environment variable not set\n");
    return 1;
  }
  logDir = std::string(home) + "/.cache/betterwallpaper";
#endif

  bwp::utils::Logger::init(logDir);
  LOG_INFO("Starting System Tray...");

  auto tray = bwp::tray::TrayIconFactory::create();
  tray->run();
  return 0;
}
