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
        LOG_INFO("Setting wallpaper on Windows: " + path);
        std::wstring wPath(path.begin(), path.end());
        BOOL result = SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)wPath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        if (!result) {
            LOG_ERROR("Failed to set wallpaper. Error code: " + std::to_string(GetLastError()));
            return false;
        }
        return true;
    }
    bool setWallpaper(const std::string& path) { return setWallpaper(path, ""); }
};
}  
