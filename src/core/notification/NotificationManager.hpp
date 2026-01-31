#pragma once
#include <functional>
#include <string>

namespace bwp::core {

// Notification severity levels
enum class NotificationType { Info, Success, Warning, Error };

/**
 * Manages system and in-app notifications.
 */
class NotificationManager {
public:
  using ToastCallback =
      std::function<void(const std::string &title, const std::string &message,
                         NotificationType type)>;

  static NotificationManager &getInstance();

  // Initialize (must be called once at startup)
  bool initialize(const std::string &appName = "BetterWallpaper");
  void shutdown();

  // Send system notification (via libnotify)
  void sendSystemNotification(const std::string &title,
                              const std::string &message,
                              NotificationType type = NotificationType::Info,
                              int timeoutMs = 5000);

  // Send in-app toast notification
  void sendToast(const std::string &title, const std::string &message,
                 NotificationType type = NotificationType::Info);

  // Enable/disable notifications
  void setSystemNotificationsEnabled(bool enabled) {
    m_systemEnabled = enabled;
  }
  void setToastsEnabled(bool enabled) { m_toastsEnabled = enabled; }
  bool isSystemNotificationsEnabled() const { return m_systemEnabled; }
  bool isToastsEnabled() const { return m_toastsEnabled; }

  // Set callback for toast display (GUI will implement display)
  void setToastCallback(ToastCallback callback) { m_toastCallback = callback; }

  // Load/save settings
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

} // namespace bwp::core
