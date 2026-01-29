#pragma once
#include <cstdint>
#include <functional>
#include <gio/gio.h>
#include <memory>
#include <string>
#include <vector>

// Forward decls to avoid full GIO headers in HPP if possible, but usually
// typedefs
struct _GDBusConnection;
typedef struct _GDBusConnection GDBusConnection;
struct _GDBusNodeInfo;
typedef struct _GDBusNodeInfo GDBusNodeInfo;

struct _GVariant;
typedef struct _GVariant GVariant;
struct _GDBusMethodInvocation;
typedef struct _GDBusMethodInvocation GDBusMethodInvocation;
struct _GError;
typedef struct _GError GError;

namespace bwp::ipc {

class DBusService {
public:
  DBusService();
  ~DBusService();

  bool initialize();
  void run(); // Main loop if standalone, or integrates with GLib loop

  // Signal emitters
  void emitWallpaperChanged(const std::string &monitor,
                            const std::string &path);
  void emitProfileChanged(const std::string &name);

  // Handler callbacks setters
  using SetWallpaperHandler = std::function<bool(std::string, std::string)>;
  void setSetWallpaperHandler(SetWallpaperHandler handler);

  using GetWallpaperHandler = std::function<std::string(std::string)>;
  void setGetWallpaperHandler(GetWallpaperHandler handler);

  using VoidMonitorHandler = std::function<void(std::string)>;
  void setNextHandler(VoidMonitorHandler handler);
  void setPreviousHandler(VoidMonitorHandler handler);
  void setPauseHandler(VoidMonitorHandler handler);
  void setResumeHandler(VoidMonitorHandler handler);
  void setStopHandler(VoidMonitorHandler handler);

  // ... other handlers

private:
  static void onBusAcquired(GDBusConnection *connection, const char *name,
                            void *user_data);
  static void onNameAcquired(GDBusConnection *connection, const char *name,
                             void *user_data);
  static void onNameLost(GDBusConnection *connection, const char *name,
                         void *user_data);

  // GDBusInterfaceVTable methods
  static void handle_method_call(GDBusConnection *connection,
                                 const char *sender, const char *object_path,
                                 const char *interface_name,
                                 const char *method_name, GVariant *parameters,
                                 GDBusMethodInvocation *invocation,
                                 void *user_data);
  static GVariant *handle_get_property(GDBusConnection *connection,
                                       const char *sender,
                                       const char *object_path,
                                       const char *interface_name,
                                       const char *property_name,
                                       GError **error, void *user_data);

  uint32_t m_ownerId = 0;
  GDBusConnection *m_connection = nullptr;
  GDBusNodeInfo *m_introspectionData = nullptr;

  SetWallpaperHandler m_setWallpaperHandler;
  GetWallpaperHandler m_getWallpaperHandler;

  VoidMonitorHandler m_nextHandler;
  VoidMonitorHandler m_prevHandler;
  VoidMonitorHandler m_pauseHandler;
  VoidMonitorHandler m_resumeHandler;
  VoidMonitorHandler m_stopHandler;
};

} // namespace bwp::ipc
