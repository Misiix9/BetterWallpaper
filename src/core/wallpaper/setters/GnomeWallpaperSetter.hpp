#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/SafeProcess.hpp"
#include "../../utils/Logger.hpp"
namespace bwp::wallpaper {
class GnomeWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        std::string uri = "file://" + path;
        auto resDark = utils::SafeProcess::exec(
            {"gsettings", "set", "org.gnome.desktop.background", "picture-uri-dark", uri});
        auto resLight = utils::SafeProcess::exec(
            {"gsettings", "set", "org.gnome.desktop.background", "picture-uri", uri});
        bool success = resDark.success() && resLight.success();
        if (success) {
            LOG_INFO("GnomeSetter: Set wallpaper " + path);
        } else {
            LOG_ERROR("GnomeSetter: Failed to set wallpaper");
        }
        return success;
    }
    bool isSupported() const override {
        const char* currentDesktop = std::getenv("XDG_CURRENT_DESKTOP");
        if (currentDesktop && std::string(currentDesktop).find("GNOME") != std::string::npos) {
             return utils::SafeProcess::commandExists("gsettings");
        }
        return false;
    }
    std::string getName() const override { return "GNOME"; }
};
}  
