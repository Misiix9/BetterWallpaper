#include "ThemeApplier.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include <cstdlib>
#include <fstream>
#include <thread>

namespace bwp::theming {

ThemeApplier &ThemeApplier::getInstance() {
  static ThemeApplier instance;
  return instance;
}

std::string ThemeApplier::toolToString(ThemeTool tool) {
  switch (tool) {
  case ThemeTool::Pywal:
    return "pywal";
  case ThemeTool::Matugen:
    return "matugen";
  case ThemeTool::Wpgtk:
    return "wpgtk";
  case ThemeTool::CustomScript:
    return "custom";
  default:
    return "none";
  }
}

ThemeTool ThemeApplier::stringToTool(const std::string &str) {
  if (str == "pywal")
    return ThemeTool::Pywal;
  if (str == "matugen")
    return ThemeTool::Matugen;
  if (str == "wpgtk")
    return ThemeTool::Wpgtk;
  if (str == "custom")
    return ThemeTool::CustomScript;
  return ThemeTool::None;
}

bool ThemeApplier::isToolAvailable(const std::string &toolName) {
  std::string cmd = "which " + toolName + " > /dev/null 2>&1";
  return system(cmd.c_str()) == 0;
}

std::vector<ThemeTool> ThemeApplier::detectAvailableTools() {
  std::vector<ThemeTool> tools;

  if (isToolAvailable("wal")) {
    tools.push_back(ThemeTool::Pywal);
  }
  if (isToolAvailable("matugen")) {
    tools.push_back(ThemeTool::Matugen);
  }
  if (isToolAvailable("wpg")) {
    tools.push_back(ThemeTool::Wpgtk);
  }

  // Custom script is always "available" if configured
  if (!m_customScript.empty()) {
    tools.push_back(ThemeTool::CustomScript);
  }

  return tools;
}

void ThemeApplier::applyFromWallpaper(const std::string &wallpaperPath,
                                      ThemeTool tool, ApplyCallback callback) {
  std::thread([this, wallpaperPath, tool, callback]() {
    bool success = false;
    std::string message;

    switch (tool) {
    case ThemeTool::Pywal:
      success = applyWithPywal(wallpaperPath);
      message = success ? "Applied theme with pywal" : "pywal failed";
      break;
    case ThemeTool::Matugen:
      success = applyWithMatugen(wallpaperPath);
      message = success ? "Applied theme with matugen" : "matugen failed";
      break;
    case ThemeTool::Wpgtk:
      success = applyWithWpgtk(wallpaperPath);
      message = success ? "Applied theme with wpgtk" : "wpgtk failed";
      break;
    case ThemeTool::CustomScript: {
      ColorPalette palette =
          ColorExtractor::getInstance().extractFromImage(wallpaperPath);
      success = applyWithCustomScript(wallpaperPath, palette);
      message =
          success ? "Applied theme with custom script" : "Custom script failed";
      break;
    }
    default:
      message = "No theming tool selected";
      break;
    }

    LOG_INFO(message);

    if (callback) {
      callback(success, message);
    }
  }).detach();
}

void ThemeApplier::applyFromPalette(const ColorPalette &palette, ThemeTool tool,
                                    ApplyCallback callback) {
  if (tool == ThemeTool::CustomScript) {
    std::thread([this, palette, callback]() {
      bool success = applyWithCustomScript("", palette);
      if (callback) {
        callback(success,
                 success ? "Applied palette" : "Failed to apply palette");
      }
    }).detach();
  } else {
    if (callback) {
      callback(false, "Tool requires wallpaper path, not palette");
    }
  }
}

bool ThemeApplier::applyWithPywal(const std::string &wallpaperPath) {
  std::string cmd = "wal -i \"" + wallpaperPath + "\" -n -q";
  LOG_DEBUG("Running: " + cmd);
  return system(cmd.c_str()) == 0;
}

bool ThemeApplier::applyWithMatugen(const std::string &wallpaperPath) {
  std::string cmd = "matugen image \"" + wallpaperPath + "\"";
  LOG_DEBUG("Running: " + cmd);
  return system(cmd.c_str()) == 0;
}

bool ThemeApplier::applyWithWpgtk(const std::string &wallpaperPath) {
  std::string cmd =
      "wpg -a \"" + wallpaperPath + "\" && wpg -s \"" + wallpaperPath + "\"";
  LOG_DEBUG("Running: " + cmd);
  return system(cmd.c_str()) == 0;
}

bool ThemeApplier::applyWithCustomScript(const std::string &wallpaperPath,
                                         const ColorPalette &palette) {
  if (m_customScript.empty()) {
    LOG_ERROR("No custom script configured");
    return false;
  }

  // Export colors as environment variables and run script
  std::string cmd = "export WALLPAPER=\"" + wallpaperPath + "\"; ";
  cmd += "export COLOR_PRIMARY=\"" + palette.primary.toHex() + "\"; ";
  cmd += "export COLOR_SECONDARY=\"" + palette.secondary.toHex() + "\"; ";
  cmd += "export COLOR_ACCENT=\"" + palette.accent.toHex() + "\"; ";
  cmd += "export COLOR_BACKGROUND=\"" + palette.background.toHex() + "\"; ";
  cmd += "export COLOR_FOREGROUND=\"" + palette.foreground.toHex() + "\"; ";

  // Export all colors
  for (size_t i = 0; i < palette.allColors.size() && i < 16; ++i) {
    cmd += "export COLOR" + std::to_string(i) + "=\"" +
           palette.allColors[i].toHex() + "\"; ";
  }

  cmd += "\"" + m_customScript + "\"";

  LOG_DEBUG("Running custom script: " + m_customScript);
  return system(cmd.c_str()) == 0;
}

void ThemeApplier::loadSettings() {
  auto &conf = bwp::config::ConfigManager::getInstance();

  std::string toolStr = conf.get<std::string>("theming.tool");
  m_preferredTool = stringToTool(toolStr);

  m_customScript = conf.get<std::string>("theming.custom_script");
  m_autoApply = conf.get<bool>("theming.auto_apply");
}

void ThemeApplier::saveSettings() {
  auto &conf = bwp::config::ConfigManager::getInstance();

  conf.set("theming.tool", toolToString(m_preferredTool));
  conf.set("theming.custom_script", m_customScript);
  conf.set("theming.auto_apply", m_autoApply);
}

} // namespace bwp::theming
