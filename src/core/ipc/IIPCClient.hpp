#pragma once
#include <string>
namespace bwp::ipc {
class IIPCClient {
public:
    virtual ~IIPCClient() = default;
    virtual bool connect() = 0;
    virtual void nextWallpaper(const std::string &monitor) = 0;
    virtual void previousWallpaper(const std::string &monitor) = 0;
    virtual void pauseWallpaper(const std::string &monitor) = 0;
    virtual void resumeWallpaper(const std::string &monitor) = 0;
    virtual void stopWallpaper(const std::string &monitor) = 0;
    virtual void setVolume(const std::string &monitor, int volume) = 0;
    virtual void setMuted(const std::string &monitor, bool muted) = 0;
    virtual bool setWallpaper(const std::string &path, const std::string &monitor) = 0;
    virtual std::string getWallpaper(const std::string &monitor) = 0;
    virtual std::string getDaemonVersion() = 0;
    virtual std::string getStatus() = 0;
    virtual std::string getMonitors() = 0;
};
}  
