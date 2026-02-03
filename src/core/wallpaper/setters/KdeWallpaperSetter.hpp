#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/ProcessUtils.hpp"
#include "../../utils/Logger.hpp"

namespace bwp::wallpaper {

class KdeWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        // KDE Plasma requires a DBus script to set wallpaper.
        // Similar to the pattern used by other tools.
        
        std::string script = 
            "var allDesktops = desktops();"
            "for (i=0;i<allDesktops.length;i++) {"
            "    d = allDesktops[i];"
            "    d.wallpaperPlugin = \"org.kde.image\";"
            "    d.currentConfigGroup = Array(\"Wallpaper\", \"org.kde.image\", \"General\");"
            "    d.writeConfig(\"Image\", \"file://" + path + "\");"
            "}";
            
        std::string cmd = "qdbus org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript '" + script + "'";
        
        auto res = utils::ProcessUtils::run(cmd);
        
        if (res.success()) {
            LOG_INFO("KdeWallpaperSetter: Set wallpaper via Plasma Scripting");
            return true;
        } else {
            LOG_ERROR("KdeWallpaperSetter: Failed: " + res.stdErr);
            return false;
        }
    }

    bool isSupported() const override {
        const char* currentDesktop = std::getenv("XDG_CURRENT_DESKTOP");
        bool isKde = (currentDesktop && std::string(currentDesktop).find("KDE") != std::string::npos);
        return isKde && utils::ProcessUtils::commandExists("qdbus");
    }

    std::string getName() const override { return "KDE"; }
};

} // namespace bwp::wallpaper
