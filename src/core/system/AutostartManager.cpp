#include "AutostartManager.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SafeProcess.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
namespace bwp::core {
AutostartManager &AutostartManager::getInstance() {
  static AutostartManager instance;
  return instance;
}
std::string AutostartManager::methodToString(AutostartMethod method) {
  switch (method) {
  case AutostartMethod::XDGAutostart:
    return "xdg";
  case AutostartMethod::SystemdUser:
    return "systemd";
  case AutostartMethod::HyprlandExecOnce:
    return "hyprland";
  default:
    return "none";
  }
}
AutostartMethod AutostartManager::stringToMethod(const std::string &str) {
  if (str == "xdg")
    return AutostartMethod::XDGAutostart;
  if (str == "systemd")
    return AutostartMethod::SystemdUser;
  if (str == "hyprland")
    return AutostartMethod::HyprlandExecOnce;
  return AutostartMethod::None;
}
std::vector<AutostartMethod> AutostartManager::getAvailableMethods() {
  std::vector<AutostartMethod> methods;
  methods.push_back(AutostartMethod::XDGAutostart);
  auto statusResult =
      bwp::utils::SafeProcess::exec({"systemctl", "--user", "status"});
  if (statusResult.exitCode == 0) {
    methods.push_back(AutostartMethod::SystemdUser);
  }
  const char *xdg_session = std::getenv("XDG_CURRENT_DESKTOP");
  if (xdg_session &&
      std::string(xdg_session).find("Hyprland") != std::string::npos) {
    methods.push_back(AutostartMethod::HyprlandExecOnce);
  }
  return methods;
}
std::string AutostartManager::getExecutablePath() const {
  if (fs::exists("/usr/bin/betterwallpaper")) {
    return "/usr/bin/betterwallpaper";
  }
  if (fs::exists("/usr/local/bin/betterwallpaper")) {
    return "/usr/local/bin/betterwallpaper";
  }
  char buf[1024];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len != -1) {
    buf[len] = '\0';
    return std::string(buf);
  }
  return "betterwallpaper";
}
std::string AutostartManager::getXDGAutostartPath() const {
  const char *configHome = std::getenv("XDG_CONFIG_HOME");
  std::string base;
  if (configHome) {
    base = configHome;
  } else {
    const char *home = std::getenv("HOME");
    base = std::string(home ? home : "") + "/.config";
  }
  return base + "/autostart/betterwallpaper.desktop";
}
std::string AutostartManager::getSystemdServicePath() const {
  const char *configHome = std::getenv("XDG_CONFIG_HOME");
  std::string base;
  if (configHome) {
    base = configHome;
  } else {
    const char *home = std::getenv("HOME");
    base = std::string(home ? home : "") + "/.config";
  }
  return base + "/systemd/user/betterwallpaper.service";
}
bool AutostartManager::enableXDGAutostart() {
  std::string path = getXDGAutostartPath();
  fs::path dir = fs::path(path).parent_path();
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  }
  std::ofstream file(path);
  if (!file) {
    LOG_ERROR("Failed to create autostart file: " + path);
    return false;
  }
  std::string exec = getExecutablePath();
  if (m_startMinimized) {
    exec += " --minimized";
  }
  file << "[Desktop Entry]\n";
  file << "Type=Application\n";
  file << "Name=BetterWallpaper\n";
  file << "Comment=Wallpaper Manager for Hyprland\n";
  file << "Exec=" << exec << "\n";
  file << "Icon=betterwallpaper\n";
  file << "Terminal=false\n";
  file << "Categories=Utility;\n";
  file << "X-GNOME-Autostart-enabled=true\n";
  file.close();
  LOG_INFO("Created XDG autostart entry: " + path);
  return true;
}
bool AutostartManager::enableSystemdUser() {
  std::string path = getSystemdServicePath();
  fs::path dir = fs::path(path).parent_path();
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  }
  std::ofstream file(path);
  if (!file) {
    LOG_ERROR("Failed to create systemd service: " + path);
    return false;
  }
  std::string exec = getExecutablePath();
  if (m_startMinimized) {
    exec += " --minimized";
  }
  file << "[Unit]\n";
  file << "Description=BetterWallpaper - Wallpaper Manager for Hyprland\n";
  file << "After=graphical-session.target\n";
  file << "\n";
  file << "[Service]\n";
  file << "Type=simple\n";
  file << "ExecStart=" << exec << "\n";
  file << "Restart=on-failure\n";
  file << "\n";
  file << "[Install]\n";
  file << "WantedBy=default.target\n";
  file.close();
  auto enableResult = bwp::utils::SafeProcess::exec(
      {"systemctl", "--user", "enable", "betterwallpaper.service"});
  if (enableResult.exitCode != 0) {
    LOG_WARN("Failed to enable systemd service");
  }
  LOG_INFO("Created systemd user service: " + path);
  return true;
}
bool AutostartManager::disableXDGAutostart() {
  std::string path = getXDGAutostartPath();
  if (fs::exists(path)) {
    fs::remove(path);
    LOG_INFO("Removed XDG autostart entry");
  }
  return true;
}
bool AutostartManager::disableSystemdUser() {
  bwp::utils::SafeProcess::exec(
      {"systemctl", "--user", "disable", "betterwallpaper.service"});
  std::string path = getSystemdServicePath();
  if (fs::exists(path)) {
    fs::remove(path);
    LOG_INFO("Removed systemd user service");
  }
  return true;
}
bool AutostartManager::enable(AutostartMethod method) {
  bool success = false;
  switch (method) {
  case AutostartMethod::XDGAutostart:
    success = enableXDGAutostart();
    break;
  case AutostartMethod::SystemdUser:
    success = enableSystemdUser();
    break;
  case AutostartMethod::HyprlandExecOnce:
    LOG_INFO("To enable Hyprland autostart, add to hyprland.conf:\n"
             "exec-once = " +
             getExecutablePath());
    success = true;
    break;
  default:
    break;
  }
  if (success) {
    m_currentMethod = method;
    saveSettings();
  }
  return success;
}
bool AutostartManager::disable() {
  disableXDGAutostart();
  disableSystemdUser();
  m_currentMethod = AutostartMethod::None;
  saveSettings();
  return true;
}
bool AutostartManager::isEnabled() const {
  if (fs::exists(getXDGAutostartPath())) {
    return true;
  }
  if (fs::exists(getSystemdServicePath())) {
    return true;
  }
  return false;
}
void AutostartManager::loadSettings() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  std::string methodStr = conf.get<std::string>("autostart.method");
  m_currentMethod = stringToMethod(methodStr);
  m_startMinimized = conf.get<bool>("general.start_minimized");
}
void AutostartManager::saveSettings() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  conf.set("autostart.method", methodToString(m_currentMethod));
  conf.set("general.start_minimized", m_startMinimized);
}
} // namespace bwp::core
