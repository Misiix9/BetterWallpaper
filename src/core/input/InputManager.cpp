#include "InputManager.hpp"
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

  // Attach to window (capture phase to ensure we catch shortcuts?)
  // Default phase is bubble.
  gtk_widget_add_controller(GTK_WIDGET(window), controller);
  LOG_INFO("InputManager: Attached keyboard controller");
}

gboolean InputManager::onKeyPressed(GtkEventControllerKey *controller,
                                    guint keyval, guint keycode,
                                    GdkModifierType state) {
  // Check modifiers (mask minimal set)
  bool ctrl = (state & GDK_CONTROL_MASK);

  GtkWidget *widget =
      gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
  GtkWindow *window = GTK_WINDOW(widget);

  if (ctrl && keyval == GDK_KEY_Right) {
    LOG_INFO("Input: Next Wallpaper");
    actionNextWallpaper();
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_Left) {
    LOG_INFO("Input: Prev Wallpaper");
    actionPrevWallpaper();
    return TRUE;
  }
  if (ctrl && (keyval == GDK_KEY_w || keyval == GDK_KEY_W)) {
    LOG_INFO("Input: Hide Window");
    actionHideWindow(window);
    return TRUE;
  }
  if (keyval == GDK_KEY_space &&
      !(state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK))) {
    // Only if not focused on a text entry?
    // Gtk handles distinct focus, but global window handler might catch it
    // first if capture phase, or last if bubble phase. If a text box has focus,
    // it usually consumes Space. We are in bubble phase (default). So if a text
    // box doesn't handle it, we get it. But text boxes DO handle space. So this
    // is safe.
    LOG_INFO("Input: Toggle Pause");
    actionTogglePause();
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
  // Start logic if totally stopped? Hard to guess context.
  // Logic: If paused, resume. If running, pause.
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

} // namespace bwp::input
