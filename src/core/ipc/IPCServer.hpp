#pragma once
#include <gtk/gtk.h>
#include <string>
namespace bwp::ipc {
class IPCServer {
public:
  static IPCServer &getInstance();
  void initialize();
private:
  IPCServer();
  ~IPCServer();
  guint m_ownerId = 0;
  static void onBusAcquired(GBusType type, GDBusConnection *connection,
                            gpointer user_data);
  static void onNameAcquired(GDBusConnection *connection, const gchar *name,
                             gpointer user_data);
  static void onNameLost(GDBusConnection *connection, const gchar *name,
                         gpointer user_data);
  static void onMethodCall(GDBusConnection *connection, const gchar *sender,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *method_name, GVariant *parameters,
                           GDBusMethodInvocation *invocation,
                           gpointer user_data);
};
}  
