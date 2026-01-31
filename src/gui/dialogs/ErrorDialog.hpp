#pragma once
#include <adwaita.h>
#include <gtk/gtk.h>
#include <string>

namespace bwp::gui {

class ErrorDialog {
public:
  static void show(GtkWindow *parent, const std::string &message,
                   const std::string &details = "") {
    AdwMessageDialog *dialog = ADW_MESSAGE_DIALOG(
        adw_message_dialog_new(parent, "Error", message.c_str()));

    if (!details.empty()) {
      adw_message_dialog_set_body(dialog, details.c_str());
    }

    adw_message_dialog_add_response(dialog, "ok", "OK");
    adw_message_dialog_set_default_response(dialog, "ok");
    adw_message_dialog_set_close_response(dialog, "ok");

    g_signal_connect(dialog, "response", G_CALLBACK(onResponse), nullptr);

    gtk_window_present(GTK_WINDOW(dialog));
  }

private:
  static void onResponse(AdwMessageDialog *dialog, char *response,
                         gpointer user_data) {
    // Just close
    // gtk_window_destroy(GTK_WINDOW(dialog)); // Adwaita handles this usually?
    // Actually for AdwMessageDialog we might need to verify lifecycle
    // But defaults usually work.
  }
};

} // namespace bwp::gui
