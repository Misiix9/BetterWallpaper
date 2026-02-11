#pragma once
#include "ITrayIcon.hpp"
#include "../core/utils/Logger.hpp"
namespace bwp::tray {
class WindowsTrayIcon : public ITrayIcon {
public:
    void run() override {
        LOG_INFO("WindowsTrayIcon::run() - Not fully implemented yet");
    }
};
}  
