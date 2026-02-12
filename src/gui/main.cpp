#include "../core/utils/Logger.hpp"
#include "Application.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc, char **argv) {
  // check for version flag before firing up the logger or anything heavy
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--version") {
      std::cout << "BetterWallpaper v" << BWP_VERSION << std::endl;
      return 0;
    }
  }

  try {
    const char *envHome = std::getenv("HOME");
    std::filesystem::path logDir = std::filesystem::current_path();

    // fallback to current dir if HOME isn't set (weird environment?)
    if (envHome) {
      std::filesystem::path home(envHome);
      logDir = home / ".config" / "betterwallpaper";
    }

    bwp::utils::Logger::init(logDir);
    LOG_SCOPE_FUNCTION("main");

    LOG_INFO("==========================================");
    LOG_INFO("   BetterWallpaper Runtime Started");
    LOG_INFO("==========================================");

    auto *app = bwp::gui::Application::create();

    // actually run the gtk app
    int status = app->run(argc, argv);

    delete app;
    return status;
  } catch (const std::exception &e) {
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
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
