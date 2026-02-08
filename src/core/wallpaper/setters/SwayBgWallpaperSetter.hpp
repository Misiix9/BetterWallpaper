#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/SafeProcess.hpp"
#include "../../utils/Logger.hpp"

namespace bwp::wallpaper {

class SwayBgWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        // Kill previous swaybg instance (safe — no shell interpolation)
        utils::SafeProcess::exec({"pkill", "-x", "swaybg"});

        // Build argument list for swaybg
        std::vector<std::string> args = {"swaybg", "-i", path, "-m", "fill"};
        if (!monitor.empty()) {
            args.push_back("-o");
            args.push_back(monitor);
        }

        // swaybg needs to stay running — launch detached
        bool success = utils::SafeProcess::execDetached(args);

        if (success) {
            LOG_INFO("SwayBgSetter: Started swaybg for " + path);
        } else {
            LOG_ERROR("SwayBgSetter: Failed to start swaybg");
        }
        return success;
    }

    bool isSupported() const override {
        const char* wayland = std::getenv("WAYLAND_DISPLAY");
        return (wayland != nullptr) && utils::SafeProcess::commandExists("swaybg");
    }

    std::string getName() const override { return "SwayBG"; }
};

} // namespace bwp::wallpaper
