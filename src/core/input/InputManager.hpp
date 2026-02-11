#pragma once
#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <unordered_map>
namespace bwp::input {
class InputManager {
public:
  static InputManager &getInstance() {
    static InputManager instance;
    return instance;
  }
  void setup(GtkWindow *window);
  gboolean onKeyPressed(GtkEventControllerKey *controller, guint keyval,
                        guint keycode, GdkModifierType state);
private:
  InputManager() = default;
  void actionNextWallpaper();
  void actionPrevWallpaper();
  void actionTogglePause();
  void actionHideWindow(GtkWindow *window);
};
}  
