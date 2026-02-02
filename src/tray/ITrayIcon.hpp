#pragma once
#include <memory>

namespace bwp::tray {

class ITrayIcon {
public:
    virtual ~ITrayIcon() = default;

    /**
     * @brief Run the tray icon main loop.
     * This blocks the thread until the application quits.
     */
    virtual void run() = 0;
};

} // namespace bwp::tray
