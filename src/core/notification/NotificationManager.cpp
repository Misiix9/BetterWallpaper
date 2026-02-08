#include "NotificationManager.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SafeProcess.hpp"

// libnotify is optional - we'll check at runtime
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#ifndef _WIN32
#include <gio/gio.h>
#endif

namespace bwp::core {

NotificationManager &NotificationManager::getInstance() {
  static NotificationManager instance;
  return instance;
}

NotificationManager::~NotificationManager() { shutdown(); }

bool NotificationManager::initialize(const std::string &appName) {
  if (m_initialized)
    return true;

#ifdef HAVE_LIBNOTIFY
  if (!notify_init(appName.c_str())) {
    LOG_WARN("Failed to initialize libnotify");
    return false;
  }
  m_initialized = true;
  LOG_INFO("NotificationManager initialized with libnotify");
#else
#ifdef _WIN32
    m_initialized = true;
    LOG_INFO("NotificationManager initialized (Windows stub)");
#else
  // Fallback: use GNotification (GIO)
  m_initialized = true;
  LOG_INFO("NotificationManager initialized (libnotify not available, using "
           "GNotification)");
#endif
#endif

  loadSettings();
  return m_initialized;
}

void NotificationManager::shutdown() {
#ifdef HAVE_LIBNOTIFY
  if (m_initialized) {
    notify_uninit();
  }
#endif
  m_initialized = false;
}

std::string NotificationManager::typeToIconName(NotificationType type) {
  switch (type) {
  case NotificationType::Success:
    return "emblem-ok-symbolic";
  case NotificationType::Warning:
    return "dialog-warning-symbolic";
  case NotificationType::Error:
    return "dialog-error-symbolic";
  case NotificationType::Info:
  default:
    return "dialog-information-symbolic";
  }
}

void NotificationManager::sendSystemNotification(const std::string &title,
                                                 const std::string &message,
                                                 NotificationType type,
                                                 int timeoutMs) {
  if (!m_systemEnabled)
    return;

#ifdef _WIN32
    LOG_INFO("[Windows Notification] " + title + ": " + message);
    // Could use Toast Notification API here later
#else
#ifdef HAVE_LIBNOTIFY
  if (!m_initialized) {
    initialize();
  }

  NotifyNotification *notification = notify_notification_new(
      title.c_str(), message.c_str(), typeToIconName(type).c_str());

  notify_notification_set_timeout(notification, timeoutMs);
  notify_notification_set_urgency(
      notification, type == NotificationType::Error ? NOTIFY_URGENCY_CRITICAL
                    : type == NotificationType::Warning ? NOTIFY_URGENCY_NORMAL
                                                        : NOTIFY_URGENCY_LOW);

  GError *error = nullptr;
  if (!notify_notification_show(notification, &error)) {
    LOG_ERROR("Failed to show notification: " +
              std::string(error ? error->message : "unknown error"));
    if (error)
      g_error_free(error);
  }

  g_object_unref(notification);
#else
  // Fallback: use notify-send command (safe — no shell interpolation)
  std::string urgency = "normal";
  if (type == NotificationType::Error)
    urgency = "critical";
  else if (type == NotificationType::Warning)
    urgency = "normal";
  else
    urgency = "low";

  utils::SafeProcess::execDetached(
      {"notify-send", "-u", urgency,
       "-t", std::to_string(timeoutMs),
       "-i", typeToIconName(type),
       title, message});
#endif
#endif
  LOG_DEBUG("System notification: " + title);
}

void NotificationManager::sendToast(const std::string &title,
                                    const std::string &message,
                                    NotificationType type) {
  if (!m_toastsEnabled)
    return;

  if (m_toastCallback) {
    m_toastCallback(title, message, type);
  } else {
    // Fallback to system notification
    sendSystemNotification(title, message, type, 3000);
  }

  LOG_DEBUG("Toast: " + title);
}

void NotificationManager::loadSettings() {
  auto &conf = bwp::config::ConfigManager::getInstance();

  // Get values with defaults (ConfigManager returns default if key doesn't
  // exist)
  m_systemEnabled = conf.get<bool>("notifications.system_enabled");
  m_toastsEnabled = conf.get<bool>("notifications.toasts_enabled");

  // Schema provides defaults, but ensure we have sane values
  // (ConfigManager get<bool> returns false by default which is fine)
}

void NotificationManager::saveSettings() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  conf.set("notifications.system_enabled", m_systemEnabled);
  conf.set("notifications.toasts_enabled", m_toastsEnabled);
}

} // namespace bwp::core
