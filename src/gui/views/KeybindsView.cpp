#include "KeybindsView.hpp"
#include "../../core/input/KeybindManager.hpp"
#include "../../core/utils/Logger.hpp"
namespace bwp::gui {
KeybindsView::KeybindsView() {
  setupUi();
  populateList();
  bwp::input::KeybindManager::getInstance().setChangeCallback([this]() {
    populateList();
  });
}
void KeybindsView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(m_box, 24);
  gtk_widget_set_margin_end(m_box, 24);
  gtk_widget_set_margin_top(m_box, 24);
  gtk_widget_set_margin_bottom(m_box, 24);
  GtkWidget *header = gtk_label_new("Keyboard Shortcuts");
  gtk_widget_add_css_class(header, "title-2");
  gtk_widget_set_halign(header, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_box), header);
  GtkWidget *subtitle = gtk_label_new("Click on a shortcut to change it, or click the X to remove it.");
  gtk_widget_add_css_class(subtitle, "dim-label");
  gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_box), subtitle);
  GtkWidget *frame = gtk_frame_new(nullptr);
  gtk_widget_add_css_class(frame, "view");
  gtk_widget_set_vexpand(frame, TRUE);
  m_listBox = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(m_listBox), GTK_SELECTION_NONE);
  gtk_widget_add_css_class(m_listBox, "boxed-list");
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), m_listBox);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_frame_set_child(GTK_FRAME(frame), scrolled);
  gtk_box_append(GTK_BOX(m_box), frame);
  GtkWidget *resetBtn = gtk_button_new_with_label("Reset to Defaults");
  gtk_widget_add_css_class(resetBtn, "destructive-action");
  gtk_widget_set_halign(resetBtn, GTK_ALIGN_END);
  g_signal_connect(resetBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer) {
    bwp::input::KeybindManager::getInstance().resetToDefaults();
  }), nullptr);
  gtk_box_append(GTK_BOX(m_box), resetBtn);
}
void KeybindsView::populateList() {
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(m_listBox)) != nullptr) {
    gtk_list_box_remove(GTK_LIST_BOX(m_listBox), child);
  }
  auto& keybinds = bwp::input::KeybindManager::getInstance().getKeybinds();
  for (const auto& kb : keybinds) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), kb.displayName.c_str());
    std::string shortcutStr = (kb.keyval == 0) ? "Not Set" : kb.toString();
    GtkWidget *shortcutBtn = gtk_button_new_with_label(shortcutStr.c_str());
    gtk_widget_add_css_class(shortcutBtn, "flat");
    gtk_widget_set_valign(shortcutBtn, GTK_ALIGN_CENTER);
    std::string *actionPtr = new std::string(kb.action);
    g_object_set_data_full(G_OBJECT(shortcutBtn), "action", actionPtr, 
      [](gpointer d) { delete static_cast<std::string*>(d); });
    g_object_set_data(G_OBJECT(shortcutBtn), "view", this);
    g_signal_connect(shortcutBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
      std::string* action = static_cast<std::string*>(g_object_get_data(G_OBJECT(btn), "action"));
      KeybindsView* view = static_cast<KeybindsView*>(g_object_get_data(G_OBJECT(btn), "view"));
      if (action && view) {
        view->onCaptureKeybind(*action);
      }
    }), nullptr);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), shortcutBtn);
    GtkWidget *deleteBtn = gtk_button_new_from_icon_name("edit-clear-symbolic");
    gtk_widget_add_css_class(deleteBtn, "flat");
    gtk_widget_set_valign(deleteBtn, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(deleteBtn, "Clear shortcut");
    std::string *delActionPtr = new std::string(kb.action);
    g_object_set_data_full(G_OBJECT(deleteBtn), "action", delActionPtr, 
      [](gpointer d) { delete static_cast<std::string*>(d); });
    g_signal_connect(deleteBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
      std::string* action = static_cast<std::string*>(g_object_get_data(G_OBJECT(btn), "action"));
      if (action) {
        bwp::input::KeybindManager::getInstance().removeKeybind(*action);
      }
    }), nullptr);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), deleteBtn);
    gtk_list_box_append(GTK_LIST_BOX(m_listBox), row);
  }
}
void KeybindsView::onCaptureKeybind(const std::string& action) {
  m_pendingAction = action;
  GtkWidget *root = gtk_widget_get_root(m_box);
  GtkWindow *parentWindow = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
  m_captureDialog = adw_message_dialog_new(parentWindow, "Press a key combination", 
    "Press the key combination you want to assign, or Escape to cancel.");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(m_captureDialog), "cancel", "Cancel");
  GtkEventController *keyController = gtk_event_controller_key_new();
  g_object_set_data(G_OBJECT(keyController), "action", new std::string(action));
  g_object_set_data(G_OBJECT(keyController), "dialog", m_captureDialog);
  g_signal_connect(keyController, "key-pressed", G_CALLBACK(+[](
    GtkEventControllerKey*, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
    GtkEventController* ctrl = GTK_EVENT_CONTROLLER(data);
    std::string* action = static_cast<std::string*>(g_object_get_data(G_OBJECT(ctrl), "action"));
    GtkWidget* dialog = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(ctrl), "dialog"));
    if (keyval == GDK_KEY_Escape) {
      gtk_window_close(GTK_WINDOW(dialog));
      return TRUE;
    }
    if (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R ||
        keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R ||
        keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R ||
        keyval == GDK_KEY_Super_L || keyval == GDK_KEY_Super_R) {
      return FALSE;
    }
    GdkModifierType mask = static_cast<GdkModifierType>(
      GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK | GDK_SUPER_MASK);
    GdkModifierType cleanState = static_cast<GdkModifierType>(state & mask);
    if (action) {
      bwp::input::KeybindManager::getInstance().setKeybind(*action, keyval, cleanState);
      delete action;
      g_object_set_data(G_OBJECT(ctrl), "action", nullptr);
    }
    gtk_window_close(GTK_WINDOW(dialog));
    return TRUE;
  }), keyController);
  gtk_widget_add_controller(m_captureDialog, keyController);
  gtk_window_present(GTK_WINDOW(m_captureDialog));
}
}  
