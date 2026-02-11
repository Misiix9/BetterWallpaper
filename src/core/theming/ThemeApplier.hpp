#pragma once
#include "ColorExtractor.hpp"
#include <functional>
#include <string>
#include <vector>
namespace bwp::theming {
enum class ThemeTool { None, Pywal, Matugen, Wpgtk, CustomScript };
class ThemeApplier {
public:
  using ApplyCallback =
      std::function<void(bool success, const std::string &message)>;
  static ThemeApplier &getInstance();
  std::vector<ThemeTool> detectAvailableTools();
  static std::string toolToString(ThemeTool tool);
  static ThemeTool stringToTool(const std::string &str);
  void applyFromWallpaper(const std::string &wallpaperPath, ThemeTool tool,
                          ApplyCallback callback = nullptr);
  void applyFromPalette(const ColorPalette &palette, ThemeTool tool,
                        ApplyCallback callback = nullptr);
  void setCustomScript(const std::string &path) { m_customScript = path; }
  std::string getCustomScript() const { return m_customScript; }
  void setPreferredTool(ThemeTool tool) { m_preferredTool = tool; }
  ThemeTool getPreferredTool() const { return m_preferredTool; }
  void setAutoApply(bool enable) { m_autoApply = enable; }
  bool isAutoApplyEnabled() const { return m_autoApply; }
  void loadSettings();
  void saveSettings();
private:
  ThemeApplier() = default;
  ~ThemeApplier() = default;
  ThemeApplier(const ThemeApplier &) = delete;
  ThemeApplier &operator=(const ThemeApplier &) = delete;
  bool applyWithPywal(const std::string &wallpaperPath);
  bool applyWithMatugen(const std::string &wallpaperPath);
  bool applyWithWpgtk(const std::string &wallpaperPath);
  bool applyWithCustomScript(const std::string &wallpaperPath,
                             const ColorPalette &palette);
  bool isToolAvailable(const std::string &toolName);
  ThemeTool m_preferredTool = ThemeTool::None;
  std::string m_customScript;
  bool m_autoApply = false;
};
}  
