#pragma once
#include "../IWallpaperSetter.hpp"
#include "../../utils/Logger.hpp"

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#endif

namespace bwp::wallpaper {

class WindowsWallpaperSetter : public IWallpaperSetter {
public:
    bool setWallpaper(const std::string& path, const std::string& monitor) override {
#ifdef _WIN32
        // monitor argument is ignored for basic SPI_SETDESKWALLPAPER as it sets for all or primary.
        // Windows 8+ supports per-monitor but requires IDesktopWallpaper COM interface.
        // Phase 6 Goal is "Native .exe", starting with basic support.
        
        // SystemParametersInfoA for ANSI path, or W for WideChar.
        // Better to convert to wstring for path support.
        
        LOG_INFO("WindowsWallpaperSetter: Setting wallpaper to " + path);
        
        // Convert std::string to std::wstring
        int len;
        int slength = (int)path.length() + 1;
        len = MultiByteToWideChar(CP_ACP, 0, path.c_str(), slength, 0, 0); 
        std::wstring wpath(len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, path.c_str(), slength, &wpath[0], len);
        
        BOOL result = SystemParametersInfoW(
            SPI_SETDESKWALLPAPER,
            0,
            (void*)wpath.c_str(),
            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE
        );
        
        if (result) {
            LOG_INFO("WindowsWallpaperSetter: Success");
            return true;
        } else {
            LOG_ERROR("WindowsWallpaperSetter: Failed with error code " + std::to_string(GetLastError()));
            return false;
        }
#else
        LOG_ERROR("WindowsWallpaperSetter: Not running on Windows!");
        return false;
#endif
    }

    bool isSupported() override {
#ifdef _WIN32
        return true;
#else
        return false;
#endif
    }

    std::string getName() const override { return "WindowsNative"; }
};

} // namespace bwp::wallpaper
