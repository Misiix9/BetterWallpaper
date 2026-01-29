#include "../core/ipc/DBusClient.hpp"
#include <iostream>
#include <string>
#include <vector>

void printUsage() {
  std::cout << "Usage: bwp <command> [args] [--monitor <name>]\n"
            << "Commands:\n"
            << "  set <path>   Set wallpaper\n"
            << "  get          Get current wallpaper path\n"
            << "  next         Skip to next wallpaper\n"
            << "  prev         Go to previous wallpaper\n"
            << "  pause        Pause playback\n"
            << "  resume       Resume playback\n"
            << "  stop         Stop playback\n"
            << "  version      Show daemon version\n"
            << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printUsage();
    return 1;
  }

  std::string command = argv[1];
  std::string monitor = "eDP-1"; // Default monitor
  std::string path = "";

  // Simple argument parsing
  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--monitor" && i + 1 < argc) {
      monitor = argv[++i];
    } else if (path.empty() && command == "set") {
      path = arg;
    }
  }

  bwp::ipc::DBusClient client;
  if (!client.connect()) {
    std::cerr << "Error: Could not connect to BetterWallpaper daemon."
              << std::endl;
    return 1;
  }

  if (command == "set") {
    if (path.empty()) {
      std::cerr << "Error: 'set' requires a wallpaper path." << std::endl;
      return 1;
    }
    if (client.setWallpaper(path, monitor)) {
      std::cout << "Wallpaper set on " << monitor << std::endl;
    } else {
      std::cerr << "Failed to set wallpaper." << std::endl;
      return 1;
    }
  } else if (command == "get") {
    std::string p = client.getWallpaper(monitor);
    if (p.empty())
      std::cout << "No wallpaper set or daemon error." << std::endl;
    else
      std::cout << p << std::endl;
  } else if (command == "next") {
    client.nextWallpaper(monitor);
  } else if (command == "prev") {
    client.previousWallpaper(monitor);
  } else if (command == "pause") {
    client.pauseWallpaper(monitor);
  } else if (command == "resume") {
    client.resumeWallpaper(monitor);
  } else if (command == "stop") {
    client.stopWallpaper(monitor);
  } else if (command == "version") {
    std::cout << client.getDaemonVersion() << std::endl;
  } else {
    printUsage();
    return 1;
  }

  return 0;
}
