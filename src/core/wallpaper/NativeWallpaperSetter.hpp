#pragma once
#include <string>
#include <vector>

namespace bwp::wallpaper {

/**
 * Native wallpaper setter using system tools.
 * Provides fallback methods when internal rendering fails.
 */
class NativeWallpaperSetter {
public:
  enum class Method {
    Swaybg,     // Universal Wayland
    Hyprctl,    // Hyprland native
    Swww,       // Another Wayland option
    Gnome,      // Gnome D-Bus
    Kde,        // KDe Plasma D-Bus
    Feh,        // X11
    Xwallpaper, // X11 alternative
    Internal    // Our gtk4-layer-shell window
  };

  static NativeWallpaperSetter &getInstance();

  // Set wallpaper using best available method
  bool setWallpaper(const std::string &imagePath,
                    const std::string &monitor = "");

  // Get available methods on this system
  std::vector<Method> getAvailableMethods();
  static std::string methodToString(Method method);

  // Get detected monitor names
  std::vector<std::string> getMonitors();

  // Force a specific method
  void setPreferredMethod(Method method) { m_preferredMethod = method; }
  Method getPreferredMethod() const { return m_preferredMethod; }

private:
  NativeWallpaperSetter() { detectMethods(); }
  ~NativeWallpaperSetter() = default;

  void detectMethods();
  bool isCommandAvailable(const std::string &cmd);

  bool setWithSwaybg(const std::string &path, const std::string &monitor);
  bool setWithHyprctl(const std::string &path, const std::string &monitor);
  bool setWithSwww(const std::string &path, const std::string &monitor);
  bool setWithFeh(const std::string &path);
  bool setWithXwallpaper(const std::string &path);

  std::vector<Method> m_availableMethods;
  Method m_preferredMethod = Method::Internal;
};

} // namespace bwp::wallpaper
