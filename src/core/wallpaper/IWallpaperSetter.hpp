#pragma once
#include <string>
#include <vector>
namespace bwp::wallpaper {
class IWallpaperSetter {
public:
    virtual ~IWallpaperSetter() = default;
    virtual bool setWallpaper(const std::string& path, const std::string& monitor = "") = 0;
    virtual bool isSupported() const = 0;
    virtual std::string getName() const = 0;
};
}  
