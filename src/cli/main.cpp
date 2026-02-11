#include "../core/ipc/IPCClientFactory.hpp"
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

static void printUsage() {
  std::cout
      << "BetterWallpaper CLI v" << BWP_VERSION << "\n\n"
      << "Usage: bwp <command> [args] [--monitor <name>]\n\n"
      << "Commands:\n"
      << "  set <path>       Set wallpaper from file path\n"
      << "  get              Get current wallpaper path\n"
      << "  next             Skip to next wallpaper in slideshow\n"
      << "  prev             Go to previous wallpaper in slideshow\n"
      << "  pause            Pause playback\n"
      << "  resume           Resume playback\n"
      << "  stop             Stop playback\n"
      << "  volume <0-100>   Set volume level\n"
      << "  mute             Mute audio\n"
      << "  unmute           Unmute audio\n"
      << "  status           Show daemon status (add --json for raw JSON)\n"
      << "  list-monitors    List available monitors\n"
      << "  version          Show daemon version\n\n"
      << "Options:\n"
      << "  --monitor <name> Target a specific monitor (e.g. DP-1)\n"
      << "  --json           Output status as raw JSON\n"
      << "  --help, -h       Show this help message\n"
      << "  --version, -v    Show version information\n"
      << std::endl;
}

static void printStatus(const std::string &jsonStr) {
  try {
    auto j = nlohmann::json::parse(jsonStr);

    std::cout << "Daemon version: "
              << j.value("daemon_version", "unknown") << "\n";

    // Slideshow info
    bool slideshowRunning = j.value("slideshow_running", false);
    bool slideshowPaused = j.value("slideshow_paused", false);
    if (slideshowRunning || slideshowPaused) {
      std::cout << "Slideshow: "
                << (slideshowPaused ? "paused" : "running")
                << " (" << j.value("slideshow_index", 0) + 1
                << "/" << j.value("slideshow_count", 0) << ")"
                << " interval=" << j.value("slideshow_interval", 0) << "s"
                << (j.value("slideshow_shuffle", false) ? " [shuffle]" : "")
                << "\n";
    } else {
      std::cout << "Slideshow: inactive\n";
    }

    // Per-monitor info
    if (j.contains("monitors") && j["monitors"].is_array()) {
      std::cout << "\nMonitors:\n";
      for (const auto &mon : j["monitors"]) {
        std::string name = mon.value("name", "?");
        std::string wp = mon.value("wallpaper", "");
        std::cout << "  " << name << ": ";
        if (wp.empty()) {
          std::cout << "(none)";
        } else {
          std::cout << wp;
        }
        std::cout << "\n";
      }
    }
  } catch (const nlohmann::json::exception &e) {
    std::cerr << "Error parsing status: " << e.what() << std::endl;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printUsage();
    return 1;
  }

  std::string command = argv[1];

  // Handle flags before connecting to daemon
  if (command == "--help" || command == "-h") {
    printUsage();
    return 0;
  }
  if (command == "--version" || command == "-v") {
    std::cout << "bwp (CLI) " << BWP_VERSION << std::endl;
    // Also try to get daemon version
    auto client = bwp::ipc::IPCClientFactory::createClient();
    if (client && client->connect()) {
      std::cout << "daemon    " << client->getDaemonVersion() << std::endl;
    } else {
      std::cout << "daemon    (not running)" << std::endl;
    }
    return 0;
  }

  // Parse arguments
  std::string monitor;
  std::string path;
  bool jsonOutput = false;
  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--monitor" && i + 1 < argc) {
      monitor = argv[++i];
    } else if (arg == "--json") {
      jsonOutput = true;
    } else if (path.empty() && (command == "set" || command == "volume")) {
      path = arg;
    }
  }

  // Connect to daemon
  auto client = bwp::ipc::IPCClientFactory::createClient();
  if (!client || !client->connect()) {
    std::cerr << "Error: Could not connect to BetterWallpaper daemon.\n"
              << "Is betterwallpaper-daemon running?" << std::endl;
    return 1;
  }

  // Command dispatch
  if (command == "set") {
    if (path.empty()) {
      std::cerr << "Error: 'set' requires a wallpaper path.\n"
                << "Usage: bwp set <path> [--monitor <name>]" << std::endl;
      return 1;
    }
    // Resolve to absolute path
    std::filesystem::path fsPath(path);
    std::error_code ec;
    auto absPath = std::filesystem::absolute(fsPath, ec);
    if (ec) {
      std::cerr << "Error: Invalid path: " << path << std::endl;
      return 1;
    }
    if (!std::filesystem::exists(absPath)) {
      std::cerr << "Error: File not found: " << absPath.string() << std::endl;
      return 1;
    }
    if (client->setWallpaper(absPath.string(), monitor)) {
      if (monitor.empty()) {
        std::cout << "Wallpaper set successfully" << std::endl;
      } else {
        std::cout << "Wallpaper set on " << monitor << std::endl;
      }
    } else {
      std::cerr << "Error: Failed to set wallpaper." << std::endl;
      return 1;
    }
  } else if (command == "get") {
    std::string p = client->getWallpaper(monitor);
    if (p.empty()) {
      std::cerr << "No wallpaper set" << std::endl;
      return 1;
    }
    std::cout << p << std::endl;
  } else if (command == "next") {
    client->nextWallpaper(monitor);
    std::cout << "Skipped to next wallpaper" << std::endl;
  } else if (command == "prev") {
    client->previousWallpaper(monitor);
    std::cout << "Returned to previous wallpaper" << std::endl;
  } else if (command == "pause") {
    client->pauseWallpaper(monitor);
    std::cout << "Playback paused" << std::endl;
  } else if (command == "resume") {
    client->resumeWallpaper(monitor);
    std::cout << "Playback resumed" << std::endl;
  } else if (command == "stop") {
    client->stopWallpaper(monitor);
    std::cout << "Playback stopped" << std::endl;
  } else if (command == "volume") {
    if (path.empty()) {
      std::cerr << "Error: 'volume' requires a level (0-100).\n"
                << "Usage: bwp volume <0-100> [--monitor <name>]" << std::endl;
      return 1;
    }
    int vol;
    try {
      vol = std::stoi(path);
    } catch (...) {
      std::cerr << "Error: Invalid volume level: " << path << std::endl;
      return 1;
    }
    if (vol < 0 || vol > 100) {
      std::cerr << "Error: Volume must be between 0 and 100" << std::endl;
      return 1;
    }
    client->setVolume(monitor, vol);
    std::cout << "Volume set to " << vol << std::endl;
  } else if (command == "mute") {
    client->setMuted(monitor, true);
    std::cout << "Audio muted" << std::endl;
  } else if (command == "unmute") {
    client->setMuted(monitor, false);
    std::cout << "Audio unmuted" << std::endl;
  } else if (command == "status") {
    std::string statusJson = client->getStatus();
    if (jsonOutput) {
      std::cout << statusJson << std::endl;
    } else {
      printStatus(statusJson);
    }
  } else if (command == "list-monitors") {
    std::string monitorsJson = client->getMonitors();
    try {
      auto arr = nlohmann::json::parse(monitorsJson);
      if (arr.empty()) {
        std::cout << "No monitors detected" << std::endl;
      } else {
        for (const auto &name : arr) {
          std::cout << name.get<std::string>() << std::endl;
        }
      }
    } catch (const nlohmann::json::exception &e) {
      std::cerr << "Error parsing monitor list: " << e.what() << std::endl;
      return 1;
    }
  } else if (command == "version") {
    std::cout << "bwp (CLI) " << BWP_VERSION << "\n"
              << "daemon    " << client->getDaemonVersion() << std::endl;
  } else {
    std::cerr << "Unknown command: " << command << "\n" << std::endl;
    printUsage();
    return 1;
  }

  return 0;
}
