#pragma once

#include <string>

typedef struct _GDBusConnection GDBusConnection;

namespace bwp::ipc {

class DBusClient {
public:
  DBusClient();
  ~DBusClient();

  bool connect();
  bool setWallpaper(const std::string &path, const std::string &monitor);
  std::string getWallpaper(const std::string &monitor);

  std::string getDaemonVersion();

  void nextWallpaper(const std::string &monitor);
  void previousWallpaper(const std::string &monitor);
  void pauseWallpaper(const std::string &monitor);
  void resumeWallpaper(const std::string &monitor);
  void stopWallpaper(const std::string &monitor);

private:
  GDBusConnection *m_connection = nullptr;
};

} // namespace bwp::ipc
