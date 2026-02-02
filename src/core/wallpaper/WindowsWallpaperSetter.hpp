#pragma once
#include <windows.h>
#include "IWallpaperSetter.hpp"
#include "../utils/Logger.hpp"
#include <iostream>

namespace bwp::wallpaper {

class WindowsWallpaperSetter : public IWallpaperSetter {
public:
    WindowsWallpaperSetter() = default;
    ~WindowsWallpaperSetter() override = default;

    bool isSupported() const override { return true; }
    std::string getName() const override { return "WindowsNative"; }

    bool setWallpaper(const std::string& path, const std::string& monitor) override {
        // Basic implementation using SystemParametersInfo
        // Note: This sets the wallpaper for *all* monitors on most Windows versions unless
        // using IDesktopWallpaper (COM) which is more complex.
        
        LOG_INFO("Setting wallpaper on Windows: " + path);
        
        // Convert path to wide string
        std::wstring wPath(path.begin(), path.end());
        
        // Use IDesktopWallpaper for per-monitor support in the future?
        // For now, simpler SystemParametersInfo
        
        BOOL result = SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)wPath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        
        if (!result) {
            LOG_ERROR("Failed to set wallpaper. Error code: " + std::to_string(GetLastError()));
            return false;
        }
        
        return true;
    }

    // Unused strict methods for now
    bool setWallpaper(const std::string& path) { return setWallpaper(path, ""); }
};

} // namespace bwp::wallpaper
