#pragma once
#include "ITrayIcon.hpp"
#ifdef _WIN32
#include "WindowsTrayIcon.hpp"
#else
#include "LinuxTrayIcon.hpp"
#endif
#include <memory>
namespace bwp::tray {
class TrayIconFactory {
public:
    static std::unique_ptr<ITrayIcon> create() {
#ifdef _WIN32
        return std::make_unique<WindowsTrayIcon>();
#else
        return std::make_unique<LinuxTrayIcon>();
#endif
    }
};
}  
