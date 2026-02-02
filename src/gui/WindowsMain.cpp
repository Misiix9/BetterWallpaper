#ifdef _WIN32
#include <windows.h>
#include "windows/WindowsMainWindow.hpp"
#include "../core/ipc/IPCClientFactory.hpp"
#include "../core/utils/Logger.hpp"

// Standard WinMAin entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    
    // Init Logger
    // TODO: proper log path
    bwp::utils::Logger::init("C:\\Temp\\bwp_gui.log");
    
    auto client = bwp::ipc::IPCClientFactory::createClient();
    
    bwp::gui::WindowsMainWindow window(std::move(client));
    
    if (!window.init(hInstance, nCmdShow)) {
        return 0;
    }
    
    return window.run();
}
#else
// This file is Windows only
#endif
