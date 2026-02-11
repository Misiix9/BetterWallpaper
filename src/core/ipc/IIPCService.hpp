#pragma once
#include <functional>
#include <string>
namespace bwp::ipc {
class IIPCService {
public:
    virtual ~IIPCService() = default;
    virtual bool initialize() = 0;
    virtual void run() = 0;  
    virtual void stop() = 0;
    using BoolHandler = std::function<bool(const std::string&, const std::string&)>;
    using VoidHandler = std::function<void(const std::string&)>;
    using VolumeHandler = std::function<void(const std::string&, int)>;
    using MuteHandler = std::function<void(const std::string&, bool)>;
    using GetStringHandler = std::function<std::string(const std::string&)>;
    virtual void setSetWallpaperHandler(BoolHandler handler) = 0;
    virtual void setGetWallpaperHandler(GetStringHandler handler) = 0;
    virtual void setNextHandler(VoidHandler handler) = 0;
    virtual void setPreviousHandler(VoidHandler handler) = 0;
    virtual void setPauseHandler(VoidHandler handler) = 0;
    virtual void setResumeHandler(VoidHandler handler) = 0;
    virtual void setStopHandler(VoidHandler handler) = 0;
    virtual void setSetVolumeHandler(VolumeHandler handler) = 0;
    virtual void setSetMutedHandler(MuteHandler handler) = 0;
};
}  
