#pragma once
#include <cstdint>
#include <functional>
#include <gio/gio.h>
#include <memory>
#include <string>
#include <vector>
#include "IIPCService.hpp"
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
class DBusService : public IIPCService {
public:
  DBusService();
  ~DBusService() override;
  bool initialize() override;
  void run() override {}  
  void stop() override {}
  void emitWallpaperChanged(const std::string &monitor,
                            const std::string &path);
  void emitProfileChanged(const std::string &name);
  void setSetWallpaperHandler(BoolHandler handler) override;
  void setGetWallpaperHandler(GetStringHandler handler) override;
  void setNextHandler(VoidHandler handler) override;
  void setPreviousHandler(VoidHandler handler) override;
  void setPauseHandler(VoidHandler handler) override;
  void setResumeHandler(VoidHandler handler) override;
  void setStopHandler(VoidHandler handler) override;
  void setSetVolumeHandler(VolumeHandler handler) override;
  void setSetMutedHandler(MuteHandler handler) override;
private:
  static void onBusAcquired(GDBusConnection *connection, const char *name,
                            void *user_data);
  static void onNameAcquired(GDBusConnection *connection, const char *name,
                             void *user_data);
  static void onNameLost(GDBusConnection *connection, const char *name,
                         void *user_data);
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
  BoolHandler m_setWallpaperHandler;
  GetStringHandler m_getWallpaperHandler;
  VoidHandler m_nextHandler;
  VoidHandler m_prevHandler;
  VoidHandler m_pauseHandler;
  VoidHandler m_resumeHandler;
  VoidHandler m_stopHandler;
};
}  
