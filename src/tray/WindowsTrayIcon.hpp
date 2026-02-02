#pragma once
#include "ITrayIcon.hpp"
#include "../core/utils/Logger.hpp"

namespace bwp::tray {

class WindowsTrayIcon : public ITrayIcon {
public:
    void run() override {
        // Placeholder for Real Windows API (User32 / Shell_NotifyIcon) works.
        // For now, just logging.
        // In a real implementation, we would implement a Win32 event loop here if not using a Framework.
        LOG_INFO("WindowsTrayIcon::run() - Not fully implemented yet");
        // while(GetMessage(&msg, NULL, 0, 0)) { ... }
    }
};

} // namespace bwp::tray
