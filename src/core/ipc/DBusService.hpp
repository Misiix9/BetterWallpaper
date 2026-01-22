#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward decls to avoid full GIO headers in HPP if possible, but usually
// typedefs
struct _GDBusConnection;
typedef struct _GDBusConnection GDBusConnection;
struct _GDBusNodeInfo;
typedef struct _GDBusNodeInfo GDBusNodeInfo;

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
                                 const char *method_name, void *parameters,
                                 void *invocation, void *user_data);
  static void *handle_get_property(GDBusConnection *connection,
                                   const char *sender, const char *object_path,
                                   const char *interface_name,
                                   const char *property_name, void *error,
                                   void *user_data);

  uint32_t m_ownerId = 0;
  GDBusConnection *m_connection = nullptr;
  GDBusNodeInfo *m_introspectionData = nullptr;

  SetWallpaperHandler m_setWallpaperHandler;
};

} // namespace bwp::ipc
