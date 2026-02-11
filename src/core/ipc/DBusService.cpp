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
    <method name="GetWallpaper">
      <arg name="monitor" type="s" direction="in"/>
      <arg name="path" type="s" direction="out"/>
    </method>
    <method name="Next">
      <arg name="monitor" type="s" direction="in"/>
    </method>
    <method name="Previous">
      <arg name="monitor" type="s" direction="in"/>
    </method>
    <method name="Pause">
      <arg name="monitor" type="s" direction="in"/>
    </method>
    <method name="Resume">
      <arg name="monitor" type="s" direction="in"/>
    </method>
    <method name="Stop">
      <arg name="monitor" type="s" direction="in"/>
    </method>
    <method name="SetVolume">
      <arg name="monitor" type="s" direction="in"/>
      <arg name="volume" type="i" direction="in"/>
    </method>
    <method name="SetMuted">
      <arg name="monitor" type="s" direction="in"/>
      <arg name="muted" type="b" direction="in"/>
    </method>
    <method name="GetStatus">
      <arg name="status" type="s" direction="out"/>
    </method>
    <method name="GetMonitors">
      <arg name="monitors" type="s" direction="out"/>
    </method>
    <property name="DaemonVersion" type="s" access="read"/>
    <signal name="WallpaperChanged">
      <arg name="monitor" type="s"/>
      <arg name="path" type="s"/>
    </signal>
    <signal name="ProfileChanged">
      <arg name="name" type="s"/>
    </signal>
  </interface>
</node>
)";  
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
  static const GDBusInterfaceVTable interface_vtable = {handle_method_call,
                                                        handle_get_property,
                                                        nullptr,  
                                                        {0}};
  auto *self = static_cast<DBusService *>(user_data);
  self->m_connection = connection;

  GError *regError = nullptr;
  guint regId = g_dbus_connection_register_object(
      connection, "/com/github/BetterWallpaper",
      self->m_introspectionData->interfaces[0], &interface_vtable, self,
      nullptr, &regError);
  if (regId == 0) {
    LOG_ERROR(std::string("Failed to register D-Bus object: ") +
              (regError ? regError->message : "unknown error"));
    if (regError)
      g_error_free(regError);
  }
}
void DBusService::onNameAcquired(GDBusConnection *, const char *name, void *) {
  LOG_INFO(std::string("Acquired D-Bus name: ") + name);
}
void DBusService::onNameLost(GDBusConnection *, const char *name, void *) {
  LOG_WARN(std::string("Lost D-Bus name: ") + name);
}
void DBusService::handle_method_call(GDBusConnection *  ,
                                     const char *, const char *, const char *,
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
  } else if (method == "GetWallpaper") {
    const char *monitor;
    g_variant_get(parameters, "(&s)", &monitor);
    std::string path;
    if (self->m_getWallpaperHandler) {
      path = self->m_getWallpaperHandler(monitor);
    }
    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new("(s)", path.c_str()));
  } else if (method == "Next" || method == "Previous" || method == "Pause" ||
             method == "Resume" || method == "Stop") {
    const char *monitor;
    g_variant_get(parameters, "(&s)", &monitor);
    std::string mon(monitor);
    if (method == "Next" && self->m_nextHandler)
      self->m_nextHandler(mon);
    else if (method == "Previous" && self->m_prevHandler)
      self->m_prevHandler(mon);
    else if (method == "Pause" && self->m_pauseHandler)
      self->m_pauseHandler(mon);
    else if (method == "Resume" && self->m_resumeHandler)
      self->m_resumeHandler(mon);
    else if (method == "Stop" && self->m_stopHandler)
      self->m_stopHandler(mon);
    g_dbus_method_invocation_return_value(invocation, nullptr);
  } else if (method == "SetVolume") {
    const char *monitor;
    int volume;
    g_variant_get(parameters, "(&si)", &monitor, &volume);
    if (self->m_volumeHandler) {
      self->m_volumeHandler(monitor, volume);
    }
    g_dbus_method_invocation_return_value(invocation, nullptr);
  } else if (method == "SetMuted") {
    const char *monitor;
    gboolean muted;
    g_variant_get(parameters, "(&sb)", &monitor, &muted);
    if (self->m_muteHandler) {
      self->m_muteHandler(monitor, static_cast<bool>(muted));
    }
    g_dbus_method_invocation_return_value(invocation, nullptr);
  } else if (method == "GetStatus") {
    std::string status;
    if (self->m_getStatusHandler) {
      status = self->m_getStatusHandler();
    }
    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new("(s)", status.c_str()));
  } else if (method == "GetMonitors") {
    std::string monitors;
    if (self->m_getMonitorsHandler) {
      monitors = self->m_getMonitorsHandler();
    }
    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new("(s)", monitors.c_str()));
  } else {
    g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                          G_DBUS_ERROR_UNKNOWN_METHOD,
                                          "Unknown method: %s", method_name);
  }
}
GVariant *DBusService::handle_get_property(GDBusConnection *, const char *,
                                           const char *, const char *,
                                           const char *property_name, GError **,
                                           void *) {
  std::string prop = property_name;
  if (prop == "DaemonVersion") {
    return g_variant_new_string(BWP_VERSION);
  }
  return nullptr;
}
void DBusService::setSetWallpaperHandler(IIPCService::BoolHandler handler) {
  m_setWallpaperHandler = handler;
}
void DBusService::setGetWallpaperHandler(
    IIPCService::GetStringHandler handler) {
  m_getWallpaperHandler = handler;
}
void DBusService::setNextHandler(IIPCService::VoidHandler handler) {
  m_nextHandler = handler;
}
void DBusService::setPreviousHandler(IIPCService::VoidHandler handler) {
  m_prevHandler = handler;
}
void DBusService::setPauseHandler(IIPCService::VoidHandler handler) {
  m_pauseHandler = handler;
}
void DBusService::setResumeHandler(IIPCService::VoidHandler handler) {
  m_resumeHandler = handler;
}
void DBusService::setStopHandler(IIPCService::VoidHandler handler) {
  m_stopHandler = handler;
}
void DBusService::setSetVolumeHandler(IIPCService::VolumeHandler handler) {
  m_volumeHandler = handler;
}  
void DBusService::setSetMutedHandler(IIPCService::MuteHandler handler) {
  m_muteHandler = handler;
}  
void DBusService::setGetStatusHandler(IIPCService::NoArgStringHandler handler) {
  m_getStatusHandler = handler;
}
void DBusService::setGetMonitorsHandler(IIPCService::NoArgStringHandler handler) {
  m_getMonitorsHandler = handler;
}

void DBusService::stop() {
  if (m_ownerId > 0) {
    g_bus_unown_name(m_ownerId);
    m_ownerId = 0;
  }
  m_connection = nullptr;
  LOG_INFO("D-Bus service stopped");
}

void DBusService::emitWallpaperChanged(const std::string &monitor,
                                       const std::string &path) {
  if (!m_connection) return;
  GError *error = nullptr;
  g_dbus_connection_emit_signal(
      m_connection, nullptr, "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", "WallpaperChanged",
      g_variant_new("(ss)", monitor.c_str(), path.c_str()), &error);
  if (error) {
    LOG_ERROR(std::string("Failed to emit WallpaperChanged: ") + error->message);
    g_error_free(error);
  }
}

void DBusService::emitProfileChanged(const std::string &name) {
  if (!m_connection) return;
  GError *error = nullptr;
  g_dbus_connection_emit_signal(
      m_connection, nullptr, "/com/github/BetterWallpaper",
      "com.github.BetterWallpaper", "ProfileChanged",
      g_variant_new("(s)", name.c_str()), &error);
  if (error) {
    LOG_ERROR(std::string("Failed to emit ProfileChanged: ") + error->message);
    g_error_free(error);
  }
}

} // namespace bwp::ipc
