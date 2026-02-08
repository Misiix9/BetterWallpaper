#include "ThemeApplier.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SafeProcess.hpp"
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
  return utils::SafeProcess::commandExists(toolName);
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
  LOG_DEBUG("Running pywal for: " + wallpaperPath);
  auto res = utils::SafeProcess::exec({"wal", "-i", wallpaperPath, "-n", "-q"});
  return res.success();
}

bool ThemeApplier::applyWithMatugen(const std::string &wallpaperPath) {
  LOG_DEBUG("Running matugen for: " + wallpaperPath);
  auto res = utils::SafeProcess::exec({"matugen", "image", wallpaperPath});
  return res.success();
}

bool ThemeApplier::applyWithWpgtk(const std::string &wallpaperPath) {
  LOG_DEBUG("Running wpgtk for: " + wallpaperPath);
  auto addRes = utils::SafeProcess::exec({"wpg", "-a", wallpaperPath});
  if (!addRes.success()) return false;
  auto setRes = utils::SafeProcess::exec({"wpg", "-s", wallpaperPath});
  return setRes.success();
}

bool ThemeApplier::applyWithCustomScript(const std::string &wallpaperPath,
                                         const ColorPalette &palette) {
  if (m_customScript.empty()) {
    LOG_ERROR("No custom script configured");
    return false;
  }

  // Build environment variables for the custom script
  std::vector<std::string> envVars = {
    "WALLPAPER=" + wallpaperPath,
    "COLOR_PRIMARY=" + palette.primary.toHex(),
    "COLOR_SECONDARY=" + palette.secondary.toHex(),
    "COLOR_ACCENT=" + palette.accent.toHex(),
    "COLOR_BACKGROUND=" + palette.background.toHex(),
    "COLOR_FOREGROUND=" + palette.foreground.toHex()
  };

  // Export all indexed colors
  for (size_t i = 0; i < palette.allColors.size() && i < 16; ++i) {
    envVars.push_back("COLOR" + std::to_string(i) + "=" +
                      palette.allColors[i].toHex());
  }

  LOG_DEBUG("Running custom script: " + m_customScript);
  auto res = utils::SafeProcess::exec({m_customScript}, envVars);
  return res.success();
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
