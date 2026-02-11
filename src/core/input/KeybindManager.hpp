#pragma once
#include <functional>
#include <gdk/gdk.h>
#include <string>
#include <vector>
namespace bwp::input {
struct Keybind {
  std::string action;        
  std::string displayName;   
  guint keyval;              
  GdkModifierType modifiers;
  std::string toString() const;   
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
  std::string match(guint keyval, GdkModifierType state) const;
  using ChangeCallback = std::function<void()>;
  void setChangeCallback(ChangeCallback cb) { m_changeCallback = cb; }
private:
  KeybindManager();
  void initDefaults();
  std::vector<Keybind> m_keybinds;
  ChangeCallback m_changeCallback;
};
}  
