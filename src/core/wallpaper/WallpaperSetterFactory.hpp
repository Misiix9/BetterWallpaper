#pragma once
#include "IWallpaperSetter.hpp"
#ifdef _WIN32
#include "setters/WindowsWallpaperSetter.hpp"
#else
#include "setters/GnomeWallpaperSetter.hpp"
#include "setters/HyprlandWallpaperSetter.hpp"
#include "setters/KdeWallpaperSetter.hpp"
#include "setters/SwayBgWallpaperSetter.hpp"
#endif
#include <memory>
#include <vector>
namespace bwp::wallpaper {
class WallpaperSetterFactory {
public:
    static std::unique_ptr<IWallpaperSetter> createSetter() {
        #ifdef _WIN32
        auto win = std::make_unique<WindowsWallpaperSetter>();
        if (win->isSupported()) return win;
        #else
        auto hypr = std::make_unique<HyprlandWallpaperSetter>();
        if (hypr->isSupported()) return hypr;
        auto gnome = std::make_unique<GnomeWallpaperSetter>();
        if (gnome->isSupported()) return gnome;
        auto kde = std::make_unique<KdeWallpaperSetter>();
        if (kde->isSupported()) return kde;
        auto sway = std::make_unique<SwayBgWallpaperSetter>();
        if (sway->isSupported()) return sway;
        #endif
        return nullptr;
    }
    static std::vector<std::string> getAvailableMethodNames() {
        std::vector<std::string> methods;
        #ifdef _WIN32
        methods.push_back("WindowsNative");
        #else
        if (HyprlandWallpaperSetter().isSupported()) methods.push_back("Hyprland");
        if (GnomeWallpaperSetter().isSupported()) methods.push_back("GNOME");
        if (KdeWallpaperSetter().isSupported()) methods.push_back("KDE");
        if (SwayBgWallpaperSetter().isSupported()) methods.push_back("SwayBG");
        #endif
        return methods;
    }
};
}  
