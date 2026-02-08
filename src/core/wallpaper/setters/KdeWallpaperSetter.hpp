#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/SafeProcess.hpp"
#include "../../utils/Logger.hpp"

namespace bwp::wallpaper {

class KdeWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        // KDE Plasma requires a DBus script to set wallpaper.
        // The path is embedded in the JS script string — SafeProcess prevents
        // shell injection, but the path is still data inside the script.
        // We pass the script as a single argument to qdbus (no shell interpolation).
        std::string script =
            "var allDesktops = desktops();"
            "for (i=0;i<allDesktops.length;i++) {"
            "    d = allDesktops[i];"
            "    d.wallpaperPlugin = \"org.kde.image\";"
            "    d.currentConfigGroup = Array(\"Wallpaper\", \"org.kde.image\", \"General\");"
            "    d.writeConfig(\"Image\", \"file://" + path + "\");"
            "}";

        // Pass script as a single argv element — no shell quoting needed
        auto res = utils::SafeProcess::exec(
            {"qdbus", "org.kde.plasmashell", "/PlasmaShell",
             "org.kde.PlasmaShell.evaluateScript", script});

        if (res.success()) {
            LOG_INFO("KdeWallpaperSetter: Set wallpaper via Plasma Scripting");
            return true;
        } else {
            LOG_ERROR("KdeWallpaperSetter: Failed: " + res.stdOut);
            return false;
        }
    }

    bool isSupported() const override {
        const char* currentDesktop = std::getenv("XDG_CURRENT_DESKTOP");
        bool isKde = (currentDesktop && std::string(currentDesktop).find("KDE") != std::string::npos);
        return isKde && utils::SafeProcess::commandExists("qdbus");
    }

    std::string getName() const override { return "KDE"; }
};

} // namespace bwp::wallpaper
