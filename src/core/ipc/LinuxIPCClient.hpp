#pragma once
#include "IIPCClient.hpp"
#include <string>
typedef struct _GDBusConnection GDBusConnection;
typedef struct _GVariant GVariant;
namespace bwp::ipc {
class LinuxIPCClient : public IIPCClient {
public:
  LinuxIPCClient();
  ~LinuxIPCClient() override;
  bool connect() override;
  bool setWallpaper(const std::string &path, const std::string &monitor) override;
  std::string getWallpaper(const std::string &monitor) override;
  std::string getDaemonVersion() override;
  std::string getStatus() override;
  std::string getMonitors() override;
  void nextWallpaper(const std::string &monitor) override;
  void previousWallpaper(const std::string &monitor) override;
  void pauseWallpaper(const std::string &monitor) override;
  void resumeWallpaper(const std::string &monitor) override;
  void stopWallpaper(const std::string &monitor) override;
  void setVolume(const std::string &monitor, int volume) override;
  void setMuted(const std::string &monitor, bool muted) override;
private:
  void callAction(const char *method, GVariant *parameters);
  GDBusConnection *m_connection = nullptr;
};
}  
