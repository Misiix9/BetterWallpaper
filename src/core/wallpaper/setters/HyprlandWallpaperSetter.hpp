#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/SafeProcess.hpp"
#include "../../utils/Logger.hpp"
namespace bwp::wallpaper {
class HyprlandWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        auto preload = utils::SafeProcess::exec({"hyprctl", "hyprpaper", "preload", path});
        if (!preload.success()) {
            LOG_ERROR("HyprlandSetter: Preload failed: " + preload.stdOut);
        }
        std::string mon = monitor;
        if (mon.empty()) {
            LOG_ERROR("HyprlandSetter: No monitor specified — cannot apply wallpaper");
            return false;
        }
        auto res = utils::SafeProcess::exec({"hyprctl", "hyprpaper", "wallpaper", mon + "," + path});
        if (res.success()) {
            LOG_INFO("HyprlandSetter: Set " + path + " on " + mon);
            return true;
        } else {
            LOG_ERROR("HyprlandSetter: Failed: " + res.stdOut);
            return false;
        }
    }
    bool isSupported() const override {
        const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
        return (sig != nullptr) && utils::SafeProcess::commandExists("hyprctl");
    }
    std::string getName() const override { return "Hyprland"; }
};
}  
