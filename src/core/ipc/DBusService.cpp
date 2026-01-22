#include "DBusService.hpp"
#include "../utils/Logger.hpp"
#include <gio/gio.h>
#include <iostream>

namespace bwp::ipc {

static const char *introspection_xml = R"(
<node>
  <interface name="com.github.BetterWallpaper">
    <method name="SetWallpaper">
      <arg name="path" type="s" direction="in"/>
      <arg name="monitor" type="s" direction="in"/>
      <arg name="success" type="b" direction="out"/>
    </method>
    <property name="DaemonVersion" type="s" access="read"/>
  </interface>
</node>
)"; // Simplified for brevity in this step, should match XML file

static const GDBusInterfaceVTable interface_vtable = {
    DBusService::handle_method_call,
    (GDBusInterfaceGetPropertyFunc)DBusService::handle_get_property,
    nullptr, // set_property
    {0}};

DBusService::DBusService() {}

DBusService::~DBusService() {
  if (m_ownerId > 0) {
    g_bus_unown_name(m_ownerId);
  }
  if (m_introspectionData) {
    g_dbus_node_info_unref(m_introspectionData);
  }
}

bool DBusService::initialize() {
  m_introspectionData =
      g_dbus_node_info_new_for_xml(introspection_xml, nullptr);
  if (!m_introspectionData) {
    LOG_ERROR("Failed to parse introspection XML");
    return false;
  }

  m_ownerId = g_bus_own_name(G_BUS_TYPE_SESSION, "com.github.BetterWallpaper",
                             G_BUS_NAME_OWNER_FLAGS_NONE, onBusAcquired,
                             onNameAcquired, onNameLost, this, nullptr);
  return true;
}

void DBusService::onBusAcquired(GDBusConnection *connection, const char *,
                                void *user_data) {
  auto *self = static_cast<DBusService *>(user_data);
  self->m_connection = connection;

  g_dbus_connection_register_object(connection, "/com/github/BetterWallpaper",
                                    self->m_introspectionData->interfaces[0],
                                    &interface_vtable, self, nullptr, nullptr);
}

void DBusService::onNameAcquired(GDBusConnection *, const char *name, void *) {
  LOG_INFO(std::string("Acquired D-Bus name: ") + name);
}

void DBusService::onNameLost(GDBusConnection *, const char *name, void *) {
  LOG_WARN(std::string("Lost D-Bus name: ") + name);
}

void DBusService::handle_method_call(GDBusConnection *connection, const char *,
                                     const char *, const char *,
                                     const char *method_name,
                                     GVariant *parameters,
                                     GDBusMethodInvocation *invocation,
                                     void *user_data) {
  auto *self = static_cast<DBusService *>(user_data);
  std::string method = method_name;

  if (method == "SetWallpaper") {
    const char *path, *monitor;
    g_variant_get(parameters, "(&s&s)", &path, &monitor);

    bool success = false;
    if (self->m_setWallpaperHandler) {
      success = self->m_setWallpaperHandler(path, monitor);
    }

    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new("(b)", success));
  } else {
    g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                          G_DBUS_ERROR_UNKNOWN_METHOD,
                                          "Unknown method: %s", method_name);
  }
}

void *DBusService::handle_get_property(GDBusConnection *, const char *,
                                       const char *, const char *,
                                       const char *property_name, void *,
                                       void *) {
  std::string prop = property_name;
  if (prop == "DaemonVersion") {
    return g_variant_new_string("1.0.0");
  }
  return nullptr;
}

void DBusService::setSetWallpaperHandler(SetWallpaperHandler handler) {
  m_setWallpaperHandler = handler;
}

} // namespace bwp::ipc
