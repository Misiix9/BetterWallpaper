#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/ProcessUtils.hpp"
#include "../../utils/Logger.hpp"

namespace bwp::wallpaper {

class GnomeWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        // GNOME sets wallpaper globally usually, monitor specific is tricky without extensions.
        // Standard org.gnome.desktop.background picture-uri
        
        std::string uri = "file://" + path;
        
        // Dark mode support
        std::string cmdDark = "gsettings set org.gnome.desktop.background picture-uri-dark '" + uri + "'";
        std::string cmdLight = "gsettings set org.gnome.desktop.background picture-uri '" + uri + "'";
        
        bool success = utils::ProcessUtils::runAsync(cmdDark);
        success &= utils::ProcessUtils::runAsync(cmdLight);
        
        if (success) {
            LOG_INFO("GnomeSetter: Set wallpaper " + path);
        } else {
            LOG_ERROR("GnomeSetter: Failed to set wallpaper");
        }
        return success;
    }

    bool isSupported() override {
        // Check if gsettings is available and XDG_CURRENT_DESKTOP contains GNOME
        const char* currentDesktop = std::getenv("XDG_CURRENT_DESKTOP");
        if (currentDesktop && std::string(currentDesktop).find("GNOME") != std::string::npos) {
             return utils::ProcessUtils::commandExists("gsettings");
        }
        return false;
    }

    std::string getName() const override { return "GNOME"; }
};

} // namespace bwp::wallpaper
