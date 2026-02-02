#include "KeybindManager.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include <gdk/gdkkeysyms.h>
#include <sstream>

namespace bwp::input {

std::string Keybind::toString() const {
  std::string result;
  if (modifiers & GDK_CONTROL_MASK) result += "Ctrl+";
  if (modifiers & GDK_SHIFT_MASK) result += "Shift+";
  if (modifiers & GDK_ALT_MASK) result += "Alt+";
  if (modifiers & GDK_SUPER_MASK) result += "Super+";
  
  // Get key name
  const char* keyName = gdk_keyval_name(keyval);
  if (keyName) {
    result += keyName;
  } else {
    result += "Unknown";
  }
  return result;
}

Keybind Keybind::fromString(const std::string& action, const std::string& displayName, const std::string& combo) {
  Keybind kb;
  kb.action = action;
  kb.displayName = displayName;
  kb.modifiers = static_cast<GdkModifierType>(0);
  kb.keyval = 0;
  
  std::string remaining = combo;
  
  // Parse modifiers
  while (true) {
    if (remaining.find("Ctrl+") == 0) {
      kb.modifiers = static_cast<GdkModifierType>(kb.modifiers | GDK_CONTROL_MASK);
      remaining = remaining.substr(5);
    } else if (remaining.find("Shift+") == 0) {
      kb.modifiers = static_cast<GdkModifierType>(kb.modifiers | GDK_SHIFT_MASK);
      remaining = remaining.substr(6);
    } else if (remaining.find("Alt+") == 0) {
      kb.modifiers = static_cast<GdkModifierType>(kb.modifiers | GDK_ALT_MASK);
      remaining = remaining.substr(4);
    } else if (remaining.find("Super+") == 0) {
      kb.modifiers = static_cast<GdkModifierType>(kb.modifiers | GDK_SUPER_MASK);
      remaining = remaining.substr(6);
    } else {
      break;
    }
  }
  
  // Parse key
  kb.keyval = gdk_keyval_from_name(remaining.c_str());
  if (kb.keyval == GDK_KEY_VoidSymbol) {
    kb.keyval = 0;
  }
  
  return kb;
}

KeybindManager::KeybindManager() {
  initDefaults();
}

void KeybindManager::initDefaults() {
  m_keybinds.clear();
  
  // Default keybinds
  m_keybinds.push_back({"next_wallpaper", "Next Wallpaper", GDK_KEY_Right, GDK_CONTROL_MASK});
  m_keybinds.push_back({"prev_wallpaper", "Previous Wallpaper", GDK_KEY_Left, GDK_CONTROL_MASK});
  m_keybinds.push_back({"toggle_pause", "Play/Pause", GDK_KEY_space, static_cast<GdkModifierType>(0)});
  m_keybinds.push_back({"hide_window", "Hide Window", GDK_KEY_w, GDK_CONTROL_MASK});
}

void KeybindManager::load() {
  auto& config = bwp::config::ConfigManager::getInstance();
  
  // Try to load from config
  auto keybindsJson = config.get<std::vector<nlohmann::json>>("keybinds");
  if (keybindsJson.empty()) {
    LOG_INFO("KeybindManager: No saved keybinds, using defaults");
    return;
  }
  
  m_keybinds.clear();
  for (const auto& kb : keybindsJson) {
    Keybind keybind;
    keybind.action = kb.value("action", "");
    keybind.displayName = kb.value("displayName", "");
    keybind.keyval = kb.value("keyval", 0u);
    keybind.modifiers = static_cast<GdkModifierType>(kb.value("modifiers", 0));
    
    if (!keybind.action.empty() && keybind.keyval != 0) {
      m_keybinds.push_back(keybind);
    }
  }
  
  LOG_INFO("KeybindManager: Loaded " + std::to_string(m_keybinds.size()) + " keybinds");
}

void KeybindManager::save() {
  auto& config = bwp::config::ConfigManager::getInstance();
  
  std::vector<nlohmann::json> keybindsJson;
  for (const auto& kb : m_keybinds) {
    nlohmann::json j;
    j["action"] = kb.action;
    j["displayName"] = kb.displayName;
    j["keyval"] = kb.keyval;
    j["modifiers"] = static_cast<int>(kb.modifiers);
    keybindsJson.push_back(j);
  }
  
  config.set("keybinds", keybindsJson);
  LOG_INFO("KeybindManager: Saved " + std::to_string(m_keybinds.size()) + " keybinds");
}

void KeybindManager::resetToDefaults() {
  initDefaults();
  save();
  if (m_changeCallback) m_changeCallback();
}

void KeybindManager::setKeybind(const std::string& action, guint keyval, GdkModifierType modifiers) {
  for (auto& kb : m_keybinds) {
    if (kb.action == action) {
      kb.keyval = keyval;
      kb.modifiers = modifiers;
      save();
      if (m_changeCallback) m_changeCallback();
      return;
    }
  }
  
  // Action not found, add new? Or ignore? For now, log warning.
  LOG_WARN("KeybindManager: Unknown action: " + action);
}

void KeybindManager::removeKeybind(const std::string& action) {
  for (auto& kb : m_keybinds) {
    if (kb.action == action) {
      kb.keyval = 0;
      kb.modifiers = static_cast<GdkModifierType>(0);
      save();
      if (m_changeCallback) m_changeCallback();
      return;
    }
  }
}

std::string KeybindManager::match(guint keyval, GdkModifierType state) const {
  // Mask to only relevant modifiers
  GdkModifierType mask = static_cast<GdkModifierType>(
    GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK | GDK_SUPER_MASK
  );
  GdkModifierType cleanState = static_cast<GdkModifierType>(state & mask);
  
  for (const auto& kb : m_keybinds) {
    if (kb.keyval == 0) continue; // Unbound
    
    if (kb.keyval == keyval && kb.modifiers == cleanState) {
      return kb.action;
    }
  }
  return "";
}

} // namespace bwp::input
