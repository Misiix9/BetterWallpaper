#pragma once
#include <string>
#include <vector>
namespace bwp::core {
enum class AutostartMethod {
  None,
  XDGAutostart,     
  SystemdUser,      
  HyprlandExecOnce  
};
class AutostartManager {
public:
  static AutostartManager &getInstance();
  std::vector<AutostartMethod> getAvailableMethods();
  static std::string methodToString(AutostartMethod method);
  static AutostartMethod stringToMethod(const std::string &str);
  bool enable(AutostartMethod method);
  bool disable();
  bool isEnabled() const;
  AutostartMethod getCurrentMethod() const { return m_currentMethod; }
  std::string getExecutablePath() const;
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
}  
