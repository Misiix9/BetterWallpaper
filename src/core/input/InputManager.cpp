#include "InputManager.hpp"
#include "KeybindManager.hpp"
#include "../../core/slideshow/SlideshowManager.hpp"
#include "../../core/utils/Logger.hpp"
namespace bwp::input {
void InputManager::setup(GtkWindow *window) {
  auto controller = gtk_event_controller_key_new();
  g_signal_connect(
      controller, "key-pressed",
      G_CALLBACK(+[](GtkEventControllerKey *controller, guint keyval,
                     guint keycode, GdkModifierType state,
                     gpointer user_data) -> gboolean {
        return InputManager::getInstance().onKeyPressed(controller, keyval,
                                                        keycode, state);
      }),
      window);
  gtk_widget_add_controller(GTK_WIDGET(window), controller);
  LOG_INFO("InputManager: Attached keyboard controller");
}
gboolean InputManager::onKeyPressed(GtkEventControllerKey *controller,
                                    guint keyval, guint keycode,
                                    GdkModifierType state) {
  GtkWidget *widget =
      gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
  GtkWindow *window = GTK_WINDOW(widget);
  std::string action = KeybindManager::getInstance().match(keyval, state);
  if (action.empty()) {
    return FALSE;
  }
  LOG_INFO("Input: Action triggered: " + action);
  if (action == "next_wallpaper") {
    actionNextWallpaper();
    return TRUE;
  }
  if (action == "prev_wallpaper") {
    actionPrevWallpaper();
    return TRUE;
  }
  if (action == "toggle_pause") {
    actionTogglePause();
    return TRUE;
  }
  if (action == "hide_window") {
    actionHideWindow(window);
    return TRUE;
  }
  return FALSE;
}
void InputManager::actionNextWallpaper() {
  bwp::core::SlideshowManager::getInstance().next();
}
void InputManager::actionPrevWallpaper() {
  bwp::core::SlideshowManager::getInstance().previous();
}
void InputManager::actionTogglePause() {
  auto &sm = bwp::core::SlideshowManager::getInstance();
  if (sm.isPaused()) {
    sm.resume();
  } else if (sm.isRunning()) {
    sm.pause();
  }
}
void InputManager::actionHideWindow(GtkWindow *window) {
  if (window) {
    gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
  }
}
}  
