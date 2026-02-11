#include "WorkspaceSelectionDialog.hpp"
#include "../../core/hyprland/HyprlandManager.hpp"
#include <filesystem>
namespace bwp::gui {
WorkspaceSelectionDialog::WorkspaceSelectionDialog(
    GtkWindow *parent, const std::string &wallpaperPath)
    : m_wallpaperPath(wallpaperPath) {
  std::string filename =
      std::filesystem::path(wallpaperPath).filename().string();
  std::string title = "Set Wallpaper for Workspaces";
  m_dialog = adw_message_dialog_new(
      parent, title.c_str(), ("Select workspaces for:\n" + filename).c_str());
  setupUi();
}
WorkspaceSelectionDialog::~WorkspaceSelectionDialog() {}
void WorkspaceSelectionDialog::setupUi() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);
  GtkWidget *row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  GtkWidget *row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  for (int i = 1; i <= 10; ++i) {
    GtkWidget *check = gtk_check_button_new_with_label(
        ("Workspace " + std::to_string(i)).c_str());
    gtk_widget_set_hexpand(check, TRUE);
    m_checkboxes.push_back(check);
    if (i <= 5) {
      gtk_box_append(GTK_BOX(row1), check);
    } else {
      gtk_box_append(GTK_BOX(row2), check);
    }
  }
  gtk_box_append(GTK_BOX(box), row1);
  gtk_box_append(GTK_BOX(box), row2);
  GtkWidget *selectAllBtn = gtk_button_new_with_label("Select All");
  gtk_widget_set_halign(selectAllBtn, GTK_ALIGN_START);
  g_object_set_data(G_OBJECT(selectAllBtn), "dialog", this);
  g_signal_connect(
      selectAllBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<WorkspaceSelectionDialog *>(data);
        for (auto *check : self->m_checkboxes) {
          gtk_check_button_set_active(GTK_CHECK_BUTTON(check), TRUE);
        }
      }),
      this);
  gtk_box_append(GTK_BOX(box), selectAllBtn);
  adw_message_dialog_set_extra_child(ADW_MESSAGE_DIALOG(m_dialog), box);
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(m_dialog), "cancel",
                                  "Cancel");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(m_dialog), "apply",
                                  "Apply");
  adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(m_dialog),
                                             "apply", ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(m_dialog),
                                          "apply");
}
void WorkspaceSelectionDialog::show(Callback callback) {
  m_callback = callback;
  g_object_set_data(G_OBJECT(m_dialog), "self", this);
  g_signal_connect(
      m_dialog, "response",
      G_CALLBACK(+[](AdwMessageDialog *d, const char *response, gpointer) {
        auto *self = static_cast<WorkspaceSelectionDialog *>(
            g_object_get_data(G_OBJECT(d), "self"));
        if (g_strcmp0(response, "apply") == 0 && self->m_callback) {
          std::set<int> selected;
          for (size_t i = 0; i < self->m_checkboxes.size(); ++i) {
            if (gtk_check_button_get_active(
                    GTK_CHECK_BUTTON(self->m_checkboxes[i]))) {
              selected.insert(i + 1);
            }
          }
          for (int wsId : selected) {
            bwp::hyprland::HyprlandManager::getInstance().setWorkspaceWallpaper(
                wsId, self->m_wallpaperPath);
          }
          if (self->m_callback) {
            self->m_callback(selected);
          }
        }
        gtk_window_close(GTK_WINDOW(d));
      }),
      nullptr);
  gtk_window_present(GTK_WINDOW(m_dialog));
}
}  
