#pragma once
#include <adwaita.h>
#include <gtk/gtk.h>
#include <string>
namespace bwp::gui {
class ErrorDialog {
public:
  static void show(GtkWindow *parent, const std::string &message,
                   const std::string &details = "") {
    AdwAlertDialog *dialog = ADW_ALERT_DIALOG(
        adw_alert_dialog_new("Error", message.c_str()));
    if (!details.empty()) {
      adw_alert_dialog_set_body(dialog, details.c_str());
    }
    adw_alert_dialog_add_response(dialog, "ok", "OK");
    adw_alert_dialog_set_default_response(dialog, "ok");
    adw_alert_dialog_set_close_response(dialog, "ok");
    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
  }
};
}  
