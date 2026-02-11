#ifdef _WIN32
#include "NamedPipeService.hpp"
#include <windows.h>
#include <iostream>
#include <thread>  
#ifdef ERROR
#undef ERROR
#endif
namespace bwp::ipc {
bool NamedPipeService::initialize() {
    m_running = true;
    std::thread([this]() {
        this->listenLoop();
    }).detach(); 
    return true;
}
void NamedPipeService::listenLoop() {
    LOG_INFO("NamedPipeService: Starting listener...");
    while (m_running) {
        HANDLE hPipe = CreateNamedPipe(
            TEXT("\\\\.\\pipe\\BetterWallpaperPipe"),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            512, 512, 0, NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            LOG_ERROR("CreateNamedPipe failed, error=" + std::to_string(GetLastError()));
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        bool connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (connected) {
            char buffer[512];
            DWORD bytesRead;
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0';
                std::string message(buffer);
                if (!message.empty() && message.back() == '\n') message.pop_back();
                LOG_INFO("NamedPipeService: Received " + message);
                if (message == "NEXT") { if(m_nextHandler) m_nextHandler("primary"); }
                else if (message == "PREV") { if(m_prevHandler) m_prevHandler("primary"); }
                else if (message == "PAUSE") { if(m_pauseHandler) m_pauseHandler("primary"); }  
                else if (message == "RESUME") { if(m_resumeHandler) m_resumeHandler("primary"); }
                else if (message == "MUTE") { if(m_muteHandler) m_muteHandler("primary", true); }
                else if (message == "UNMUTE") { if(m_muteHandler) m_muteHandler("primary", false); }
            }
        }
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
}
}  
#endif
