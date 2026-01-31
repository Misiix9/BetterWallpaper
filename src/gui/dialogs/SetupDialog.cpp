#include "SetupDialog.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/wallpaper/LibraryScanner.hpp"
#include <iostream>

namespace bwp::gui {

SetupDialog::SetupDialog(GtkWindow *parent) {
  m_dialog = gtk_window_new();
  gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
  gtk_window_set_modal(GTK_WINDOW(m_dialog), TRUE);
  gtk_window_set_title(GTK_WINDOW(m_dialog), "Welcome to BetterWallpaper");
  gtk_window_set_default_size(GTK_WINDOW(m_dialog), 500, 400);

  // Main container
  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(m_dialog), mainBox);

  // Stack for pages
  m_stack = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(m_stack),
                                GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
  gtk_widget_set_vexpand(m_stack, TRUE);
  gtk_box_append(GTK_BOX(mainBox), m_stack);

  createWelcomePage();
  createLibraryPage();
  createFinishPage();
}

SetupDialog::~SetupDialog() {
  // Widgets managed by GTK
}

void SetupDialog::show() { gtk_window_present(GTK_WINDOW(m_dialog)); }

void SetupDialog::createWelcomePage() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(box, 48);
  gtk_widget_set_margin_end(box, 48);

  GtkWidget *icon =
      gtk_image_new_from_icon_name("preferences-desktop-wallpaper");
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 96);
  gtk_box_append(GTK_BOX(box), icon);

  GtkWidget *title = gtk_label_new("Welcome to BetterWallpaper");
  gtk_widget_add_css_class(title, "title-1");
  gtk_box_append(GTK_BOX(box), title);

  GtkWidget *desc = gtk_label_new(
      "Let's get your wallpaper library set up in just a few steps.");
  gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
  gtk_widget_add_css_class(desc, "body");
  gtk_box_append(GTK_BOX(box), desc);

  GtkWidget *btn = gtk_button_new_with_label("Get Started");
  gtk_widget_add_css_class(btn, "pill");
  gtk_widget_add_css_class(btn, "suggested-action");
  gtk_widget_set_halign(btn, GTK_ALIGN_CENTER);
  g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     GtkStack *stack = GTK_STACK(data);
                     gtk_stack_set_visible_child_name(stack, "library");
                   }),
                   m_stack);
  gtk_box_append(GTK_BOX(box), btn);

  gtk_stack_add_named(GTK_STACK(m_stack), box, "welcome");
}

void SetupDialog::createLibraryPage() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(box, 48);
  gtk_widget_set_margin_end(box, 48);

  GtkWidget *title = gtk_label_new("Select Library Folder");
  gtk_widget_add_css_class(title, "title-2");
  gtk_box_append(GTK_BOX(box), title);

  GtkWidget *desc =
      gtk_label_new("Choose where you want keeping your wallpapers. We'll scan "
                    "this folder recursively.");
  gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
  gtk_box_append(GTK_BOX(box), desc);

  m_libraryPathLabel = gtk_label_new("No folder selected");
  gtk_widget_add_css_class(m_libraryPathLabel, "dim-label");
  gtk_box_append(GTK_BOX(box), m_libraryPathLabel);

  GtkWidget *selectBtn = gtk_button_new_with_label("Choose Folder...");
  g_signal_connect(
      selectBtn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
        SetupDialog *self = static_cast<SetupDialog *>(data);

        GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Select Library", GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn))),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Select", "_Cancel");

        g_signal_connect(
            native, "response",
            G_CALLBACK(+[](GtkNativeDialog *dialog, int response, gpointer d) {
              SetupDialog *s = static_cast<SetupDialog *>(d);
              if (response == GTK_RESPONSE_ACCEPT) {
                GFile *file =
                    gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
                char *path = g_file_get_path(file);
                s->m_selectedPath = path;
                gtk_label_set_text(GTK_LABEL(s->m_libraryPathLabel), path);
                g_free(path);
                g_object_unref(file);
              }
              g_object_unref(dialog);
            }),
            self);

        gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
      }),
      this);
  gtk_box_append(GTK_BOX(box), selectBtn);

  // Nav buttons
  GtkWidget *navBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_halign(navBox, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(navBox, 24);

  GtkWidget *nextBtn = gtk_button_new_with_label("Next");
  gtk_widget_add_css_class(nextBtn, "suggested-action");
  g_signal_connect(
      nextBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
        SetupDialog *self = static_cast<SetupDialog *>(data);
        // Save path
        if (!self->m_selectedPath.empty()) {
          auto &conf = bwp::config::ConfigManager::getInstance();
          std::vector<std::string> paths = {self->m_selectedPath};
          conf.set("library.paths", paths);
        }
        gtk_stack_set_visible_child_name(GTK_STACK(self->m_stack), "finish");
      }),
      this);

  gtk_box_append(GTK_BOX(navBox), nextBtn);
  gtk_box_append(GTK_BOX(box), navBox);

  gtk_stack_add_named(GTK_STACK(m_stack), box, "library");
}

void SetupDialog::createFinishPage() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(box, 48);
  gtk_widget_set_margin_end(box, 48);

  GtkWidget *icon = gtk_image_new_from_icon_name("object-select-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 64);
  gtk_widget_add_css_class(icon, "success"); // Assuming css class
  gtk_box_append(GTK_BOX(box), icon);

  GtkWidget *title = gtk_label_new("You're all set!");
  gtk_widget_add_css_class(title, "title-1");
  gtk_box_append(GTK_BOX(box), title);

  GtkWidget *btn = gtk_button_new_with_label("Start Using BetterWallpaper");
  gtk_widget_add_css_class(btn, "pill");
  gtk_widget_add_css_class(btn, "suggested-action");
  gtk_widget_set_halign(btn, GTK_ALIGN_CENTER);

  g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     SetupDialog *self = static_cast<SetupDialog *>(data);

                     // Mark first run done
                     auto &conf = bwp::config::ConfigManager::getInstance();
                     conf.set("general.first_run", false);
                     conf.save(); // ensure saved

                     // Trigger scan
                     std::vector<std::string> paths =
                         conf.get<std::vector<std::string>>("library.paths");
                     bwp::wallpaper::LibraryScanner::getInstance().scan(paths);

                     gtk_window_destroy(GTK_WINDOW(self->m_dialog));
                     // Note: Self is likely deleted here if managed by caller
                     // or we need to handle lifecycle
                     delete self;
                   }),
                   this);

  gtk_box_append(GTK_BOX(box), btn);

  gtk_stack_add_named(GTK_STACK(m_stack), box, "finish");
}

} // namespace bwp::gui
