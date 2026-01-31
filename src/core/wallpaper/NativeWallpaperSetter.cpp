#include "NativeWallpaperSetter.hpp"
#include "../utils/Logger.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <unistd.h>

namespace bwp::wallpaper {

NativeWallpaperSetter &NativeWallpaperSetter::getInstance() {
  static NativeWallpaperSetter instance;
  return instance;
}

std::string NativeWallpaperSetter::methodToString(Method method) {
  switch (method) {
  case Method::Swaybg:
    return "swaybg";
  case Method::Hyprctl:
    return "hyprctl";
  case Method::Swww:
    return "swww";
  case Method::Feh:
    return "feh";
  case Method::Xwallpaper:
    return "xwallpaper";
  case Method::Internal:
    return "internal";
  }
  return "unknown";
}

bool NativeWallpaperSetter::isCommandAvailable(const std::string &cmd) {
  std::string checkCmd = "which " + cmd + " > /dev/null 2>&1";
  int result = system(checkCmd.c_str());
  bool available = (result == 0);
  LOG_DEBUG("Command check: " + cmd + " -> " +
            (available ? "available" : "NOT available"));
  return available;
}

void NativeWallpaperSetter::detectMethods() {
  m_availableMethods.clear();

  // Detect Wayland/X11
  const char *waylandDisplay = std::getenv("WAYLAND_DISPLAY");
  const char *xDisplay = std::getenv("DISPLAY");

  LOG_INFO("Environment: WAYLAND_DISPLAY=" +
           std::string(waylandDisplay ? waylandDisplay : "(null)") +
           ", DISPLAY=" + std::string(xDisplay ? xDisplay : "(null)"));
  std::cerr << "[BWP] WAYLAND_DISPLAY="
            << (waylandDisplay ? waylandDisplay : "(null)") << std::endl;

  if (waylandDisplay) {
    LOG_INFO("Detected Wayland session, checking wallpaper tools...");

    // Wayland methods
    if (isCommandAvailable("swaybg")) {
      m_availableMethods.push_back(Method::Swaybg);
      m_preferredMethod = Method::Swaybg;
      LOG_INFO("swaybg found - set as preferred method");
      std::cerr << "[BWP] swaybg found - set as preferred" << std::endl;
    }
    if (isCommandAvailable("hyprctl")) {
      m_availableMethods.push_back(Method::Hyprctl);
    }
    if (isCommandAvailable("swww")) {
      m_availableMethods.push_back(Method::Swww);
    }
  }

  if (xDisplay && !waylandDisplay) {
    if (isCommandAvailable("feh")) {
      m_availableMethods.push_back(Method::Feh);
      m_preferredMethod = Method::Feh;
    }
    if (isCommandAvailable("xwallpaper")) {
      m_availableMethods.push_back(Method::Xwallpaper);
    }
  }

  m_availableMethods.push_back(Method::Internal);

  LOG_INFO("Available methods: " + std::to_string(m_availableMethods.size()) +
           ". Preferred: " + methodToString(m_preferredMethod));
}

std::vector<NativeWallpaperSetter::Method>
NativeWallpaperSetter::getAvailableMethods() {
  return m_availableMethods;
}

std::vector<std::string> NativeWallpaperSetter::getMonitors() {
  std::vector<std::string> monitors;

  // Check Wayland
  const char *waylandDisplay = std::getenv("WAYLAND_DISPLAY");
  if (waylandDisplay) {
    // Try Hyprland (hyprctl)
    if (isCommandAvailable("hyprctl")) {
      // This is a rough parse, ideally use JSON or dedicated lib
      // hyprctl monitors -j is better but needs JSON parser.
      // Let's try simple grep for active monitors
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
    // Try Sway (swaymsg)
    else if (isCommandAvailable("swaymsg")) {
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
    }
  }
  // Check X11 (or XWayland fallback)
  else {
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

  // Fallback if none found
  if (monitors.empty()) {
    monitors.push_back("0"); // Default index
  }

  return monitors;
}

bool NativeWallpaperSetter::setWallpaper(const std::string &imagePath,
                                         const std::string &monitor) {
  std::cerr << "[BWP] === setWallpaper ===" << std::endl;
  std::cerr << "[BWP] Image: " << imagePath << std::endl;
  std::cerr << "[BWP] Monitor: " << (monitor.empty() ? "(all)" : monitor)
            << std::endl;

  LOG_INFO("=== NativeWallpaperSetter::setWallpaper ===");
  LOG_INFO("  Image: " + imagePath);
  LOG_INFO("  Monitor: " + (monitor.empty() ? "(all)" : monitor));

  if (!std::filesystem::exists(imagePath)) {
    LOG_ERROR("ERROR: Wallpaper file not found: " + imagePath);
    std::cerr << "[BWP] ERROR: File not found!" << std::endl;
    return false;
  }

  // If no methods detected yet, detect now
  if (m_availableMethods.empty()) {
    LOG_WARN("No methods detected, running detectMethods()...");
    std::cerr << "[BWP] Running detectMethods()..." << std::endl;
    detectMethods();
  }

  std::cerr << "[BWP] Preferred method: " << methodToString(m_preferredMethod)
            << std::endl;

  bool success = false;

  // Try swaybg directly if on Wayland (most reliable)
  const char *waylandDisplay = std::getenv("WAYLAND_DISPLAY");
  if (waylandDisplay && isCommandAvailable("swaybg")) {
    std::cerr << "[BWP] Trying swaybg directly..." << std::endl;
    success = setWithSwaybg(imagePath, monitor);
    if (success) {
      std::cerr << "[BWP] swaybg SUCCESS!" << std::endl;
      return true;
    }
  }

  // Try preferred method
  switch (m_preferredMethod) {
  case Method::Swaybg:
    success = setWithSwaybg(imagePath, monitor);
    break;
  case Method::Hyprctl:
    success = setWithHyprctl(imagePath, monitor);
    break;
  case Method::Swww:
    success = setWithSwww(imagePath, monitor);
    break;
  case Method::Feh:
    success = setWithFeh(imagePath);
    break;
  case Method::Xwallpaper:
    success = setWithXwallpaper(imagePath);
    break;
  case Method::Internal:
    break;
  }

  // Fallbacks
  if (!success) {
    std::cerr << "[BWP] Preferred failed, trying fallbacks..." << std::endl;
    for (auto method : m_availableMethods) {
      if (method == m_preferredMethod || method == Method::Internal)
        continue;

      switch (method) {
      case Method::Swaybg:
        success = setWithSwaybg(imagePath, monitor);
        break;
      case Method::Hyprctl:
        success = setWithHyprctl(imagePath, monitor);
        break;
      case Method::Swww:
        success = setWithSwww(imagePath, monitor);
        break;
      case Method::Feh:
        success = setWithFeh(imagePath);
        break;
      case Method::Xwallpaper:
        success = setWithXwallpaper(imagePath);
        break;
      default:
        break;
      }

      if (success) {
        std::cerr << "[BWP] Fallback succeeded: " << methodToString(method)
                  << std::endl;
        break;
      }
    }
  }

  if (!success) {
    std::cerr << "[BWP] ERROR: All methods failed!" << std::endl;
  }

  return success;
}

bool NativeWallpaperSetter::setWithSwaybg(const std::string &path,
                                          const std::string &monitor) {
  std::cerr << "[BWP] setWithSwaybg starting..." << std::endl;

  // Kill existing swaybg
  system("pkill -x swaybg 2>/dev/null");
  usleep(100000); // 100ms

  std::string cmd = "swaybg";
  if (!monitor.empty()) {
    cmd += " -o " + monitor;
  }
  cmd += " -i \"" + path + "\" -m fill";

  // Run in background with nohup
  std::string fullCmd = "nohup " + cmd + " > /tmp/swaybg.log 2>&1 &";

  std::cerr << "[BWP] Executing: " << fullCmd << std::endl;
  LOG_INFO("Executing: " + fullCmd);

  int result = system(fullCmd.c_str());
  std::cerr << "[BWP] system() returned: " << result << std::endl;

  // Wait and check
  usleep(300000); // 300ms
  int checkResult = system("pgrep -x swaybg > /dev/null 2>&1");
  bool running = (checkResult == 0);

  std::cerr << "[BWP] swaybg running: " << (running ? "YES" : "NO")
            << std::endl;

  if (!running) {
    // Check log
    std::cerr << "[BWP] Checking /tmp/swaybg.log..." << std::endl;
    system("cat /tmp/swaybg.log 2>/dev/null");
    return false;
  }

  LOG_INFO("swaybg started successfully!");
  return true;
}

bool NativeWallpaperSetter::setWithHyprctl(const std::string &path,
                                           const std::string &monitor) {
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
}

bool NativeWallpaperSetter::setWithSwww(const std::string &path,
                                        const std::string &monitor) {
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
}

bool NativeWallpaperSetter::setWithFeh(const std::string &path) {
  std::string cmd = "feh --bg-fill \"" + path + "\" 2>&1";
  std::cerr << "[BWP] Running: " << cmd << std::endl;
  return system(cmd.c_str()) == 0;
}

bool NativeWallpaperSetter::setWithXwallpaper(const std::string &path) {
  std::string cmd = "xwallpaper --zoom \"" + path + "\" 2>&1";
  std::cerr << "[BWP] Running: " << cmd << std::endl;
  return system(cmd.c_str()) == 0;
}

} // namespace bwp::wallpaper
