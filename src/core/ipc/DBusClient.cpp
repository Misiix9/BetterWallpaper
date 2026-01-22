#include "DBusClient.hpp"
#include <gio/gio.h>
#include <iostream>

namespace bwp::ipc {

DBusClient::DBusClient() {}

DBusClient::~DBusClient() {
  if (m_connection) {
    g_object_unref(m_connection);
  }
}

bool DBusClient::connect() {
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

bool DBusClient::setWallpaper(const std::string &path,
                              const std::string &monitor) {
  if (!m_connection)
    return false;

  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", "SetWallpaper",
      g_variant_new("(ss)", path.c_str(), monitor.c_str()),
      G_VARIANT_TYPE("(b)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

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

std::string DBusClient::getDaemonVersion() {
  if (!m_connection)
    return "Unknown";

  GError *error = nullptr;
  GVariant *result = g_dbus_connection_call_sync(
      m_connection, "com.github.BetterWallpaper", "/com/github/BetterWallpaper",
      "org.freedesktop.DBus.Properties", "Get",
      g_variant_new("(ss)", "com.github.BetterWallpaper", "DaemonVersion"),
      G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
  if (!result) {
    if (error)
      g_error_free(error);
    return "Unknown";
  }

  GVariant *container;
  g_variant_get(result, "(v)", &container); // Extract variant from tuple
  const char *version = g_variant_get_string(container, nullptr);
  std::string v = version ? version : "Unknown";

  g_variant_unref(container);
  g_variant_unref(result);
  return v;
}

} // namespace bwp::ipc
