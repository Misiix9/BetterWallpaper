#pragma once
#include <string>
#include <vector>

namespace bwp::core {

// Autostart methods
enum class AutostartMethod {
  None,
  XDGAutostart,    // ~/.config/autostart/
  SystemdUser,     // ~/.config/systemd/user/
  HyprlandExecOnce // Manual hyprland.conf edit
};

/**
 * Manages application autostart configuration.
 */
class AutostartManager {
public:
  static AutostartManager &getInstance();

  // Detect available methods on this system
  std::vector<AutostartMethod> getAvailableMethods();
  static std::string methodToString(AutostartMethod method);
  static AutostartMethod stringToMethod(const std::string &str);

  // Enable/disable autostart
  bool enable(AutostartMethod method);
  bool disable();

  // Check current status
  bool isEnabled() const;
  AutostartMethod getCurrentMethod() const { return m_currentMethod; }

  // Get executable path
  std::string getExecutablePath() const;

  // Settings
  void setStartMinimized(bool minimized) { m_startMinimized = minimized; }
  bool isStartMinimized() const { return m_startMinimized; }

  void loadSettings();
  void saveSettings();

private:
  AutostartManager() = default;
  ~AutostartManager() = default;

  AutostartManager(const AutostartManager &) = delete;
  AutostartManager &operator=(const AutostartManager &) = delete;

  bool enableXDGAutostart();
  bool enableSystemdUser();
  bool disableXDGAutostart();
  bool disableSystemdUser();

  std::string getXDGAutostartPath() const;
  std::string getSystemdServicePath() const;

  AutostartMethod m_currentMethod = AutostartMethod::None;
  bool m_startMinimized = false;
};

} // namespace bwp::core
