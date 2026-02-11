#include "NativeWallpaperSetter.hpp"
#include "WallpaperSetterFactory.hpp"
#include "../utils/Logger.hpp"
#include "../utils/ProcessUtils.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
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
  std::string checkCmd = "which " + cmd + " > /dev/null 2>&1";
  int result = system(checkCmd.c_str());
  bool available = (result == 0);
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
      FILE *pipe =
          popen("hyprctl monitors | grep 'Monitor' | awk '{print $2}'", "r");
      if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
          std::string mon(buffer);
          if (!mon.empty() && mon.back() == '\n')
            mon.pop_back();
          if (!mon.empty())
            monitors.push_back(mon);
        }
        pclose(pipe);
      }
    }
    else if (isCommandAvailable("swaymsg")) {
      LOG_DEBUG("Trying swaymsg...");
      FILE *pipe = popen("swaymsg -t get_outputs | grep '\"name\":' | awk "
                         "'{print $2}' | tr -d '\",'",
                         "r");
      if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
          std::string mon(buffer);
          if (!mon.empty() && mon.back() == '\n')
            mon.pop_back();
          if (!mon.empty())
            monitors.push_back(mon);
        }
        pclose(pipe);
      }
    } else {
        LOG_DEBUG("No specific Wayland compositor tools found.");
    }
  }
  else {
    LOG_DEBUG("Checking X11...");
    if (isCommandAvailable("xrandr")) {
      FILE *pipe =
          popen("xrandr --query | grep ' connected' | awk '{print $1}'", "r");
      if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
          std::string mon(buffer);
          if (!mon.empty() && mon.back() == '\n')
            mon.pop_back();
          if (!mon.empty())
            monitors.push_back(mon);
        }
        pclose(pipe);
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
  std::cerr << "[BWP] setWithSwaybg starting..." << std::endl;
  system("pkill -x swaybg 2>/dev/null");
  usleep(100000);  
  std::string cmd = "swaybg";
  if (!monitor.empty()) {
    cmd += " -o " + monitor;
  }
  cmd += " -i \"" + path + "\" -m fill";
  std::string fullCmd = "nohup " + cmd + " > /tmp/swaybg.log 2>&1 &";
  std::cerr << "[BWP] Executing: " << fullCmd << std::endl;
  LOG_INFO("Executing: " + fullCmd);
  int result = system(fullCmd.c_str());
  std::cerr << "[BWP] system() returned: " << result << std::endl;
  usleep(300000);  
  int checkResult = system("pgrep -x swaybg > /dev/null 2>&1");
  bool running = (checkResult == 0);
  std::cerr << "[BWP] swaybg running: " << (running ? "YES" : "NO")
            << std::endl;
  if (!running) {
    std::cerr << "[BWP] Checking /tmp/swaybg.log..." << std::endl;
    system("cat /tmp/swaybg.log 2>/dev/null");
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
  std::cerr << "[BWP] setWithHyprctl starting..." << std::endl;
  std::string preloadCmd = "hyprctl hyprpaper preload \"" + path + "\" 2>&1";
  std::cerr << "[BWP] Preload: " << preloadCmd << std::endl;
  int preloadResult = system(preloadCmd.c_str());
  if (preloadResult != 0) {
    std::cerr << "[BWP] Preload failed: " << preloadResult << std::endl;
    return false;
  }
  std::string monitorName = monitor.empty() ? "eDP-1" : monitor;
  std::string setCmd =
      "hyprctl hyprpaper wallpaper \"" + monitorName + "," + path + "\" 2>&1";
  std::cerr << "[BWP] Set: " << setCmd << std::endl;
  int setResult = system(setCmd.c_str());
  std::cerr << "[BWP] Set result: " << setResult << std::endl;
  return setResult == 0;
#endif
}
bool NativeWallpaperSetter::setWithSwww(const std::string &path,
                                        const std::string &monitor) {
#ifdef _WIN32
    return false;
#else
  std::cerr << "[BWP] setWithSwww starting..." << std::endl;
  system("swww init 2>/dev/null");
  usleep(200000);
  std::string cmd = "swww img";
  if (!monitor.empty()) {
    cmd += " -o " + monitor;
  }
  cmd += " \"" + path + "\" --transition-type fade 2>&1";
  std::cerr << "[BWP] Running: " << cmd << std::endl;
  int result = system(cmd.c_str());
  std::cerr << "[BWP] Result: " << result << std::endl;
  return result == 0;
#endif
}
bool NativeWallpaperSetter::setWithFeh(const std::string &path) {
#ifdef _WIN32
    return false;
#else
  std::string cmd = "feh --bg-fill \"" + path + "\" 2>&1";
  std::cerr << "[BWP] Running: " << cmd << std::endl;
  return system(cmd.c_str()) == 0;
#endif
}
bool NativeWallpaperSetter::setWithXwallpaper(const std::string &path) {
#ifdef _WIN32
    return false;
#else
  std::string cmd = "xwallpaper --zoom \"" + path + "\" 2>&1";
  std::cerr << "[BWP] Running: " << cmd << std::endl;
  return system(cmd.c_str()) == 0;
#endif
}
}  
