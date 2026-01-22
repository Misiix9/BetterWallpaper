#pragma once
#include <memory>
#include <string>

typedef struct _GDBusConnection GDBusConnection;

namespace bwp::ipc {

class DBusClient {
public:
  DBusClient();
  ~DBusClient();

  bool connect();
  bool setWallpaper(const std::string &path, const std::string &monitor);
  std::string getDaemonVersion();

private:
  GDBusConnection *m_connection = nullptr;
};

} // namespace bwp::ipc
