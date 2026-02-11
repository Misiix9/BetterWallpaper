#pragma once
#include <string>
#include <vector>
namespace bwp::wallpaper {
class NativeWallpaperSetter {
public:
  enum class Method {
    Swaybg,      
    Hyprctl,     
    Swww,        
    Gnome,       
    Kde,         
    Feh,         
    Xwallpaper,  
    Internal     
  };
  static NativeWallpaperSetter &getInstance();
  bool setWallpaper(const std::string &imagePath,
                    const std::string &monitor = "");
  std::vector<Method> getAvailableMethods();
  static std::string methodToString(Method method);
  std::vector<std::string> getMonitors();
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
}  
