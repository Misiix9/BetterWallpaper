#include "../core/utils/Logger.hpp"
#include "Application.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
  // Initialize Logger
  // Use current directory or a temp specific one for now to ensure visibility
  // passed "local file" requirement.
  try {
    const char *envHome = std::getenv("HOME");
    std::filesystem::path logDir = std::filesystem::current_path();
    if (envHome) {
      // Prefer a writable user directory if possible to avoid permission issues
      // But user asked for "local file". Let's try CWD first, but safely.
      // Actually, let's use the standard config location pattern to be safe,
      // usually ~/.config/betterwallpaper
      std::filesystem::path home(envHome);
      logDir = home / ".config" / "betterwallpaper";
    }
    bwp::utils::Logger::init(logDir);
    LOG_SCOPE_FUNCTION("main");

    LOG_INFO("==========================================");
    LOG_INFO("   BetterWallpaper Runtime Started");
    LOG_INFO("==========================================");

    auto *app = bwp::gui::Application::create();
    int status = app->run(argc, argv);
    delete app; // Ensure destructor runs for cleanup/persistence
    return status;
  } catch (const std::exception &e) {
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
    // Try to log if logger is initialized
    try {
      LOG_FATAL(std::string("Unhandled exception in main: ") + e.what());
    } catch (...) {
    }
    return 1;
  } catch (...) {
    std::cerr << "FATAL ERROR: Unknown exception" << std::endl;
    try {
      LOG_FATAL("Unhandled unknown exception in main");
    } catch (...) {
    }
    return 1;
  }
}
