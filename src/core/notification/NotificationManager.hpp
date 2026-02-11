#pragma once
#include <functional>
#include <string>
namespace bwp::core {
enum class NotificationType { Info, Success, Warning, Error };
class NotificationManager {
public:
  using ToastCallback =
      std::function<void(const std::string &title, const std::string &message,
                         NotificationType type)>;
  static NotificationManager &getInstance();
  bool initialize(const std::string &appName = "BetterWallpaper");
  void shutdown();
  void sendSystemNotification(const std::string &title,
                              const std::string &message,
                              NotificationType type = NotificationType::Info,
                              int timeoutMs = 5000);
  void sendToast(const std::string &title, const std::string &message,
                 NotificationType type = NotificationType::Info);
  void setSystemNotificationsEnabled(bool enabled) {
    m_systemEnabled = enabled;
  }
  void setToastsEnabled(bool enabled) { m_toastsEnabled = enabled; }
  bool isSystemNotificationsEnabled() const { return m_systemEnabled; }
  bool isToastsEnabled() const { return m_toastsEnabled; }
  void setToastCallback(ToastCallback callback) { m_toastCallback = callback; }
  void loadSettings();
  void saveSettings();
private:
  NotificationManager() = default;
  ~NotificationManager();
  NotificationManager(const NotificationManager &) = delete;
  NotificationManager &operator=(const NotificationManager &) = delete;
  std::string typeToIconName(NotificationType type);
  bool m_initialized = false;
  bool m_systemEnabled = true;
  bool m_toastsEnabled = true;
  ToastCallback m_toastCallback;
};
}  
