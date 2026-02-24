#include "LinuxIPCClient.hpp"
#include "../utils/Logger.hpp"
#include <gio/gio.h>
#include <iostream>
namespace bwp::ipc {
LinuxIPCClient::LinuxIPCClient() {}
LinuxIPCClient::~LinuxIPCClient() {
  // Note: m_connection from g_bus_get_sync() is a shared/cached singleton
  // managed by GLib. Do NOT unref it — doing so can prematurely free the
  // connection and cause use-after-free in other D-Bus users.
  m_connection = nullptr;
}
bool LinuxIPCClient::connect() {
  GError *error = nullptr;
  m_connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
  if (!m_connection) {
    std::cerr << "Failed to connect to D-Bus: "
              << (error ? error->message : "Unknown") << std::endl;
    if (error)
      g_error_free(error);
    return false;
  }
  return true;
}
bool LinuxIPCClient::setWallpaper(const std::string &path,
                                  const std::string &monitor) {
  if (!m_connection)
    return false;
  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", "SetWallpaper",
      g_variant_new("(ss)", path.c_str(), monitor.c_str()),
      G_VARIANT_TYPE("(b)"), G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);
  if (!result) {
    std::cerr << "Method call failed: " << (error ? error->message : "Unknown")
              << std::endl;
    if (error)
      g_error_free(error);
    return false;
  }
  bool success;
  g_variant_get(result, "(b)", &success);
  g_variant_unref(result);
  return success;
}
std::string LinuxIPCClient::getWallpaper(const std::string &monitor) {
  if (!m_connection)
    return "";
  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", "GetWallpaper",
      g_variant_new("(s)", monitor.c_str()), G_VARIANT_TYPE("(s)"),
      G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);
  if (!result) {
    if (error)
      g_error_free(error);
    return "";
  }
  const char *path;
  g_variant_get(result, "(&s)", &path);
  std::string p = path ? path : "";
  g_variant_unref(result);
  return p;
}
std::string LinuxIPCClient::getDaemonVersion() {
  if (!m_connection)
    return "Unknown";
  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "org.freedesktop.DBus.Properties", "Get",
      g_variant_new("(ss)", "com.github.BetterWallpaper", "DaemonVersion"),
      G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);
  if (!result) {
    if (error)
      g_error_free(error);
    return "Unknown";
  }
  GVariant *container;
  g_variant_get(result, "(v)", &container);
  const char *version = g_variant_get_string(container, nullptr);
  std::string v = version ? version : "Unknown";
  g_variant_unref(container);
  g_variant_unref(result);
  return v;
}
std::string LinuxIPCClient::getStatus() {
  if (!m_connection)
    return "{}";
  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", "GetStatus", nullptr, G_VARIANT_TYPE("(s)"),
      G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);
  if (!result) {
    if (error)
      g_error_free(error);
    return "{}";
  }
  const char *status;
  g_variant_get(result, "(&s)", &status);
  std::string s = status ? status : "{}";
  g_variant_unref(result);
  return s;
}
std::string LinuxIPCClient::getMonitors() {
  if (!m_connection)
    return "[]";
  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", "GetMonitors", nullptr,
      G_VARIANT_TYPE("(s)"), G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);
  if (!result) {
    if (error)
      g_error_free(error);
    return "[]";
  }
  const char *monitors;
  g_variant_get(result, "(&s)", &monitors);
  std::string m = monitors ? monitors : "[]";
  g_variant_unref(result);
  return m;
}
void LinuxIPCClient::callAction(const char *method, GVariant *parameters) {
  if (!m_connection)
    return;
  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", method, parameters, nullptr,
      G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);
  if (!result) {
    LOG_ERROR(std::string("D-Bus call '") + method +
              "' failed: " + (error ? error->message : "Unknown"));
    if (error)
      g_error_free(error);
    return;
  }
  g_variant_unref(result);
}
void LinuxIPCClient::nextWallpaper(const std::string &monitor) {
  callAction("Next", g_variant_new("(s)", monitor.c_str()));
}
void LinuxIPCClient::previousWallpaper(const std::string &monitor) {
  callAction("Previous", g_variant_new("(s)", monitor.c_str()));
}
void LinuxIPCClient::pauseWallpaper(const std::string &monitor) {
  callAction("Pause", g_variant_new("(s)", monitor.c_str()));
}
void LinuxIPCClient::resumeWallpaper(const std::string &monitor) {
  callAction("Resume", g_variant_new("(s)", monitor.c_str()));
}
void LinuxIPCClient::stopWallpaper(const std::string &monitor) {
  callAction("Stop", g_variant_new("(s)", monitor.c_str()));
}
void LinuxIPCClient::setVolume(const std::string &monitor, int volume) {
  callAction("SetVolume", g_variant_new("(si)", monitor.c_str(), volume));
}
void LinuxIPCClient::setMuted(const std::string &monitor, bool muted) {
  callAction("SetMuted", g_variant_new("(sb)", monitor.c_str(), muted));
}
} // namespace bwp::ipc
