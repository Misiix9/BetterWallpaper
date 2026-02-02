#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/ProcessUtils.hpp"
#include "../../utils/Logger.hpp"

namespace bwp::wallpaper {

class HyprlandWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        // hyprctl hyprpaper logic
        // Preload
        std::string preloadCmd = "hyprctl hyprpaper preload \"" + path + "\"";
        utils::ProcessUtils::run(preloadCmd); // Sync check?
        
        std::string mon = monitor.empty() ? "" : monitor; 
        // If monitor empty, maybe apply to all? Hyprpaper requires monitor.
        // Fallback to all detected monitors?
        
        // For now, assume monitor is passed or default
        if (mon.empty()) mon = "eDP-1"; // TODO: get from MonitorManager
        
        std::string setCmd = "hyprctl hyprpaper wallpaper \"" + mon + "," + path + "\"";
        auto res = utils::ProcessUtils::run(setCmd);
        
        // Unload unused?
        
        if (res.success()) {
            LOG_INFO("HyprlandSetter: Set " + path + " on " + mon);
            return true;
        } else {
            LOG_ERROR("HyprlandSetter: Failed: " + res.stdErr);
            return false;
        }
    }

    bool isSupported() override {
        const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
        return (sig != nullptr) && utils::ProcessUtils::commandExists("hyprctl");
    }

    std::string getName() const override { return "Hyprland"; }
};

} // namespace bwp::wallpaper
