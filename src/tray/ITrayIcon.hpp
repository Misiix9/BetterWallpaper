#pragma once
#include <memory>
namespace bwp::tray {
class ITrayIcon {
public:
    virtual ~ITrayIcon() = default;
    virtual void run() = 0;
};
}  
