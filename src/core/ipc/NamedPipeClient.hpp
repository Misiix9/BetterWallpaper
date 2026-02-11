#pragma once
#include "IIPCClient.hpp"
#include "../utils/Logger.hpp"
#ifdef _WIN32
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#endif
namespace bwp::ipc {
class NamedPipeClient : public IIPCClient {
public:
    NamedPipeClient() = default;
    ~NamedPipeClient() override = default;
    bool connect() override {
        return true; 
    }
    bool sendCommand(const std::string& cmd) {
#ifdef _WIN32
        HANDLE hPipe;
        DWORD dwWritten;
        hPipe = CreateFile(TEXT("\\\\.\\pipe\\BetterWallpaperPipe"), 
                           GENERIC_READ | GENERIC_WRITE, 
                           0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            LOG_ERROR("NamedPipeClient: Could not open pipe. GLE=" + std::to_string(GetLastError()));
            return false;
        }
        std::string message = cmd + "\n";  
        WriteFile(hPipe, message.c_str(), (DWORD)message.size(), &dwWritten, NULL);
        CloseHandle(hPipe);
        LOG_INFO("NamedPipeClient: Sent " + cmd);
        return true;
#else
        return false;
#endif
    }
    void nextWallpaper(const std::string&) override { sendCommand("NEXT"); }
    void previousWallpaper(const std::string&) override { sendCommand("PREV"); }
    void pauseWallpaper(const std::string&) override { sendCommand("PAUSE"); }
    void resumeWallpaper(const std::string&) override { sendCommand("RESUME"); }
    void stopWallpaper(const std::string&) override { sendCommand("STOP"); }
    void setVolume(const std::string&, int vol) override { sendCommand("VOLUME " + std::to_string(vol)); }
    void setMuted(const std::string&, bool muted) override { sendCommand(muted ? "MUTE" : "UNMUTE"); }
    bool setWallpaper(const std::string &path, const std::string &) override {
        return sendCommand("SET " + path); 
    }
    std::string getWallpaper(const std::string &) override { return ""; }  
    std::string getDaemonVersion() override { return "0.2.0-win"; }
};
}  
