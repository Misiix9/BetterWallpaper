#pragma once

#include <functional>
#include <gdk/gdk.h>
#include <string>
#include <vector>

namespace bwp::input {

struct Keybind {
  std::string action;       // e.g., "next_wallpaper", "pause", "hide_window"
  std::string displayName;  // e.g., "Next Wallpaper"
  guint keyval;             // GDK keyval
  GdkModifierType modifiers;
  
  std::string toString() const;  // e.g., "Ctrl+Right"
  static Keybind fromString(const std::string& action, const std::string& displayName, const std::string& combo);
};

class KeybindManager {
public:
  static KeybindManager &getInstance() {
    static KeybindManager instance;
    return instance;
  }

  void load();
  void save();
  void resetToDefaults();

  const std::vector<Keybind>& getKeybinds() const { return m_keybinds; }
  
  void setKeybind(const std::string& action, guint keyval, GdkModifierType modifiers);
  void removeKeybind(const std::string& action);
  
  // Check if a key event matches any registered keybind
  // Returns action name if matched, empty string otherwise
  std::string match(guint keyval, GdkModifierType state) const;

  // Callback for when keybinds change
  using ChangeCallback = std::function<void()>;
  void setChangeCallback(ChangeCallback cb) { m_changeCallback = cb; }

private:
  KeybindManager();
  void initDefaults();

  std::vector<Keybind> m_keybinds;
  ChangeCallback m_changeCallback;
};

} // namespace bwp::input
