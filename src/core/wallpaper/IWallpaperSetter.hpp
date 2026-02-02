#pragma once
#include <string>
#include <vector>

namespace bwp::wallpaper {

/**
 * @brief Interface for platform-specific wallpaper setting logic.
 */
class IWallpaperSetter {
public:
    virtual ~IWallpaperSetter() = default;

    /**
     * @brief Set the wallpaper for a specific monitor or all monitors.
     * @param path Absolute path to the image/video.
     * @param monitor Monitor identifier (e.g. "eDP-1", "0", or empty for all).
     * @return true if successful.
     */
    virtual bool setWallpaper(const std::string& path, const std::string& monitor = "") = 0;

    /**
     * @brief Check if this setter is supported in the current environment.
     * @return true if detected (e.g. swaybg is installed or GNOME session active).
     */
    virtual bool isSupported() const = 0;

    /**
     * @brief Get name of this setter for logging.
     * @return Name string.
     */
    virtual std::string getName() const = 0;
};

} // namespace bwp::wallpaper
