#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/ProcessUtils.hpp"
#include "../../utils/Logger.hpp"

namespace bwp::wallpaper {

class SwayBgWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        // Kill previous
        utils::ProcessUtils::run("pkill -x swaybg");
        
        std::string cmd = "swaybg -i \"" + path + "\" -m fill";
        if (!monitor.empty()) {
            cmd += " -o " + monitor;
        }
        
        // Run async in background
        // Note: swaybg needs to stay running.
        bool success = utils::ProcessUtils::runAsync(cmd);
        
        if (success) {
            LOG_INFO("SwayBgSetter: Started swaybg for " + path);
        } else {
            LOG_ERROR("SwayBgSetter: Failed to start swaybg");
        }
        return success;
    }

    bool isSupported() override {
        // Supporting sway or generic wayland fallback
        const char* wayland = std::getenv("WAYLAND_DISPLAY");
        return (wayland != nullptr) && utils::ProcessUtils::commandExists("swaybg");
    }

    std::string getName() const override { return "SwayBG"; }
};

} // namespace bwp::wallpaper
