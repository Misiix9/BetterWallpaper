#include "NativeWallpaperSetter.hpp"
#include "WallpaperSetterFactory.hpp"
#include "../monitor/MonitorManager.hpp"
#include "../utils/Logger.hpp"
#include "../utils/ProcessUtils.hpp"
#include "../utils/SafeProcess.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#define usleep(x) Sleep(x / 1000)
#endif
namespace bwp::wallpaper {
NativeWallpaperSetter &NativeWallpaperSetter::getInstance() {
  static NativeWallpaperSetter instance;
  return instance;
}
std::string NativeWallpaperSetter::methodToString(Method method) {
  switch (method) {
  case Method::Swaybg: return "swaybg";
  case Method::Hyprctl: return "hyprctl";
  case Method::Swww: return "swww";
  case Method::Feh: return "feh";
  case Method::Xwallpaper: return "xwallpaper";
  case Method::Internal: return "internal";
  default: return "unknown";
  }
}
bool NativeWallpaperSetter::isCommandAvailable(const std::string &cmd) {
#ifdef _WIN32
    return false;
#else
  bool available = utils::SafeProcess::commandExists(cmd);
  LOG_DEBUG("Command check: " + cmd + " -> " +
            (available ? "available" : "NOT available"));
  return available;
#endif
}
void NativeWallpaperSetter::detectMethods() {
  m_availableMethods.clear();
  auto names = WallpaperSetterFactory::getAvailableMethodNames();
#ifdef _WIN32
  for (const auto& name : names) {
  }
#else
  for (const auto& name : names) {
    if (name == "Hyprland") m_availableMethods.push_back(Method::Hyprctl);
    else if (name == "GNOME") m_availableMethods.push_back(Method::Gnome);
    else if (name == "KDE") m_availableMethods.push_back(Method::Kde);
    else if (name == "SwayBG") m_availableMethods.push_back(Method::Swaybg);
  }
  if (utils::ProcessUtils::commandExists("feh")) m_availableMethods.push_back(Method::Feh);
  if (utils::ProcessUtils::commandExists("xwallpaper")) m_availableMethods.push_back(Method::Xwallpaper);
#endif
  m_availableMethods.push_back(Method::Internal);
  LOG_INFO("Available methods: " + std::to_string(m_availableMethods.size()));
}
std::vector<NativeWallpaperSetter::Method>
NativeWallpaperSetter::getAvailableMethods() {
  return m_availableMethods;
}
std::vector<std::string> NativeWallpaperSetter::getMonitors() {
  LOG_DEBUG("NativeWallpaperSetter::getMonitors() start");
  std::vector<std::string> monitors;
#ifdef _WIN32
  monitors.push_back("0"); 
#else
  const char *waylandDisplay = std::getenv("WAYLAND_DISPLAY");
  if (waylandDisplay) {
    LOG_DEBUG("Wayland detected: " + std::string(waylandDisplay));
    if (isCommandAvailable("hyprctl")) {
      LOG_DEBUG("Trying hyprctl...");
      auto result = utils::SafeProcess::exec({"hyprctl", "monitors"});
      if (result.success()) {
        std::istringstream stream(result.stdOut);
        std::string line;
        while (std::getline(stream, line)) {
          // Lines like "Monitor DP-1 ..."
          if (line.find("Monitor") != std::string::npos) {
            auto spacePos = line.find(' ');
            if (spacePos != std::string::npos) {
              auto nameStart = spacePos + 1;
              auto nameEnd = line.find(' ', nameStart);
              std::string mon = (nameEnd != std::string::npos)
                ? line.substr(nameStart, nameEnd - nameStart)
                : line.substr(nameStart);
              if (!mon.empty())
                monitors.push_back(mon);
            }
          }
        }
      }
    }
    else if (isCommandAvailable("swaymsg")) {
      LOG_DEBUG("Trying swaymsg...");
      auto result = utils::SafeProcess::exec({"swaymsg", "-t", "get_outputs"});
      if (result.success()) {
        // Parse JSON-like output for "name" fields
        std::istringstream stream(result.stdOut);
        std::string line;
        while (std::getline(stream, line)) {
          auto namePos = line.find("\"name\":");
          if (namePos != std::string::npos) {
            auto valStart = line.find('"', namePos + 7);
            auto valEnd = line.find('"', valStart + 1);
            if (valStart != std::string::npos && valEnd != std::string::npos) {
              std::string mon = line.substr(valStart + 1, valEnd - valStart - 1);
              if (!mon.empty())
                monitors.push_back(mon);
            }
          }
        }
      }
    } else {
        LOG_DEBUG("No specific Wayland compositor tools found.");
    }
  }
  else {
    LOG_DEBUG("Checking X11...");
    if (isCommandAvailable("xrandr")) {
      auto result = utils::SafeProcess::exec({"xrandr", "--query"});
      if (result.success()) {
        std::istringstream stream(result.stdOut);
        std::string line;
        while (std::getline(stream, line)) {
          if (line.find(" connected") != std::string::npos) {
            auto spacePos = line.find(' ');
            if (spacePos != std::string::npos) {
              std::string mon = line.substr(0, spacePos);
              if (!mon.empty())
                monitors.push_back(mon);
            }
          }
        }
      }
    }
  }
#endif
  if (monitors.empty()) {
    LOG_DEBUG("No monitors found, adding default.");
    monitors.push_back("0");  
  }
  LOG_DEBUG("NativeWallpaperSetter::getMonitors() end");
  return monitors;
}
bool NativeWallpaperSetter::setWallpaper(const std::string &imagePath,
                                         const std::string &monitor) {
  LOG_INFO("=== NativeWallpaperSetter (Factory) ===");
  LOG_INFO("  Image: " + imagePath);
  if (!std::filesystem::exists(imagePath)) {
    LOG_ERROR("File not found: " + imagePath);
    return false;
  }
  auto setter = WallpaperSetterFactory::createSetter();
  if (!setter) {
    LOG_ERROR("No supported wallpaper setter found!");
    return false;
  }
  LOG_INFO("Using detected setter: " + setter->getName());
  return setter->setWallpaper(imagePath, monitor);
}
bool NativeWallpaperSetter::setWithSwaybg(const std::string &path,
                                          const std::string &monitor) {
#ifdef _WIN32
    return false;
#else
  LOG_INFO("setWithSwaybg starting...");
  utils::SafeProcess::exec({"pkill", "-x", "swaybg"});
  usleep(100000);  
  std::vector<std::string> args = {"swaybg"};
  if (!monitor.empty()) {
    args.push_back("-o");
    args.push_back(monitor);
  }
  args.push_back("-i");
  args.push_back(path);
  args.push_back("-m");
  args.push_back("fill");
  bool started = utils::SafeProcess::execDetached(args);
  if (!started) {
    LOG_ERROR("Failed to start swaybg");
    return false;
  }
  usleep(300000);
  auto check = utils::SafeProcess::exec({"pgrep", "-x", "swaybg"});
  if (!check.success()) {
    LOG_ERROR("swaybg not running after launch");
    return false;
  }
  LOG_INFO("swaybg started successfully!");
  return true;
#endif
}
bool NativeWallpaperSetter::setWithHyprctl(const std::string &path,
                                           const std::string &monitor) {
#ifdef _WIN32
    return false;
#else
  LOG_INFO("setWithHyprctl starting...");
  auto preload = utils::SafeProcess::exec({"hyprctl", "hyprpaper", "preload", path});
  if (!preload.success()) {
    LOG_ERROR("hyprctl preload failed: " + preload.stdErr);
    return false;
  }
  std::string monitorName = monitor;
  if (monitorName.empty()) {
    auto monitors = bwp::monitor::MonitorManager::getInstance().getMonitors();
    monitorName = monitors.empty() ? "" : monitors[0].name;
  }
  auto set = utils::SafeProcess::exec({"hyprctl", "hyprpaper", "wallpaper",
                                        monitorName + "," + path});
  if (!set.success()) {
    LOG_ERROR("hyprctl wallpaper set failed: " + set.stdErr);
    return false;
  }
  return true;
#endif
}
bool NativeWallpaperSetter::setWithSwww(const std::string &path,
                                        const std::string &monitor) {
#ifdef _WIN32
    return false;
#else
  LOG_INFO("setWithSwww starting...");
  utils::SafeProcess::exec({"swww", "init"});
  usleep(200000);
  std::vector<std::string> args = {"swww", "img"};
  if (!monitor.empty()) {
    args.push_back("-o");
    args.push_back(monitor);
  }
  args.push_back(path);
  args.push_back("--transition-type");
  args.push_back("fade");
  auto result = utils::SafeProcess::exec(args);
  if (!result.success()) {
    LOG_ERROR("swww img failed: " + result.stdErr);
  }
  return result.success();
#endif
}
bool NativeWallpaperSetter::setWithFeh(const std::string &path) {
#ifdef _WIN32
    return false;
#else
  auto result = utils::SafeProcess::exec({"feh", "--bg-fill", path});
  if (!result.success()) {
    LOG_ERROR("feh failed: " + result.stdErr);
  }
  return result.success();
#endif
}
bool NativeWallpaperSetter::setWithXwallpaper(const std::string &path) {
#ifdef _WIN32
    return false;
#else
  auto result = utils::SafeProcess::exec({"xwallpaper", "--zoom", path});
  if (!result.success()) {
    LOG_ERROR("xwallpaper failed: " + result.stdErr);
  }
  return result.success();
#endif
}
}  
