#pragma once
#include "IIPCService.hpp"
#include "../utils/Logger.hpp"
#include <thread>
#include <atomic>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace bwp::ipc {

class NamedPipeService : public IIPCService {
public:
    NamedPipeService() = default;
    ~NamedPipeService() override {
        stop();
    }

    bool initialize() override;

    void run() override {
        // Thread already running
    }

    void stop() override {
        m_running = false;
        if (m_thread.joinable()) m_thread.join();
    }

    // Handlers
    void setSetWallpaperHandler(BoolHandler h) override { m_setWallpaperHandler = h; }
    void setGetWallpaperHandler(GetStringHandler h) override { m_getWallpaperHandler = h; }
    void setNextHandler(VoidHandler h) override { m_nextHandler = h; }
    void setPreviousHandler(VoidHandler h) override { m_prevHandler = h; }
    void setPauseHandler(VoidHandler h) override { m_pauseHandler = h; }
    void setResumeHandler(VoidHandler h) override { m_resumeHandler = h; }
    void setStopHandler(VoidHandler h) override { m_stopHandler = h; }
    void setSetVolumeHandler(VolumeHandler h) override { m_volumeHandler = h; }
    void setSetMutedHandler(MuteHandler h) override { m_muteHandler = h; }

private:
    std::atomic<bool> m_running{false};
    std::thread m_thread;

    BoolHandler m_setWallpaperHandler;
    GetStringHandler m_getWallpaperHandler;
    VoidHandler m_nextHandler;
    VoidHandler m_prevHandler;
    VoidHandler m_pauseHandler;
    VoidHandler m_resumeHandler;
    VoidHandler m_stopHandler;
    VolumeHandler m_volumeHandler;
    MuteHandler m_muteHandler;

#ifdef _WIN32
    void listenLoop();
#endif
};

} // namespace bwp::ipc
