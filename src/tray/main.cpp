#include "TrayIcon.hpp"
#include "utils/Logger.hpp"
#include <gtk/gtk.h>

int main(int argc, char **argv) {
  gtk_init(&argc, &argv);

  // Initialize logger for this process
  std::string logDir = std::string(getenv("HOME")) + "/.cache/betterwallpaper";
  bwp::utils::Logger::init(logDir);

  LOG_INFO("Starting System Tray...");

  bwp::tray::TrayIcon tray;
  tray.run();

  return 0;
}
