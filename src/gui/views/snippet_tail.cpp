void WorkshopView::showSteamLoginDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

  auto *dialog = adw_message_dialog_new(window, "Steam Login", "Enter your Steam credentials to enable downloads.\n(2FA will be prompted if enabled)");
  
  GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  
  GtkWidget* userEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(userEntry), "Username");
  gtk_box_append(GTK_BOX(content), userEntry);

  GtkWidget* passEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(passEntry), "Password");
  gtk_entry_set_visibility(GTK_ENTRY(passEntry), FALSE);
  gtk_box_append(GTK_BOX(content), passEntry);

  adw_message_dialog_set_extra_child(ADW_MESSAGE_DIALOG(dialog), content);
  
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "cancel", "Cancel");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "login", "Login");
  adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "login", ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(dialog), "login");

  // Keep entries alive via data
  g_object_set_data(G_OBJECT(dialog), "user_entry", userEntry);
  g_object_set_data(G_OBJECT(dialog), "pass_entry", passEntry);

  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwMessageDialog *d, const char *response, gpointer selfPtr) {
       auto* self = static_cast<WorkshopView*>(selfPtr);
       if (g_strcmp0(response, "login") == 0) {
           GtkWidget* u = GTK_WIDGET(g_object_get_data(G_OBJECT(d), "user_entry"));
           GtkWidget* p = GTK_WIDGET(g_object_get_data(G_OBJECT(d), "pass_entry"));
           std::string user = gtk_editable_get_text(GTK_EDITABLE(u));
           std::string pass = gtk_editable_get_text(GTK_EDITABLE(p));
           
           if (!user.empty()) {
               bwp::steam::SteamService::getInstance().setSteamUser(user);
               bwp::steam::SteamService::getInstance().login(pass, 
               [self](bool success, const std::string& msg) {
                   g_idle_add(+[](gpointer data) -> gboolean {
                       auto* res = static_cast<std::pair<WorkshopView*, std::string>*>(data);
                       res->first->refreshLoginState();
                       bwp::core::utils::ToastRequest req;
                       req.message = res->second;
                       req.type = (res->second.find("success") != std::string::npos || res->second.find("OK") != std::string::npos) ? bwp::core::utils::ToastType::Success : bwp::core::utils::ToastType::Error;
                       bwp::core::utils::ToastManager::getInstance().showToast(req);
                       delete res;
                       return G_SOURCE_REMOVE;
                   }, new std::pair<WorkshopView*, std::string>(self, msg));
               },
               [self]() {
                   g_idle_add(+[](gpointer data) -> gboolean {
                       static_cast<WorkshopView*>(data)->showSteam2FADialog();
                       return G_SOURCE_REMOVE;
                   }, self);
               });
           }
       }
       gtk_window_close(GTK_WINDOW(d));
  }), this);

  gtk_window_present(GTK_WINDOW(dialog));
}

void WorkshopView::showSteam2FADialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
  
  auto *dialog = adw_message_dialog_new(window, "Steam Guard", "Please enter your 2FA code (Email or Mobile app).");
  
  GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget* codeEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(codeEntry), "XXXXX");
  gtk_box_append(GTK_BOX(content), codeEntry);
  adw_message_dialog_set_extra_child(ADW_MESSAGE_DIALOG(dialog), content);

  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "cancel", "Cancel");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "submit", "Submit");
  adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "submit", ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(dialog), "submit");
  
  g_object_set_data(G_OBJECT(dialog), "code_entry", codeEntry);
  
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwMessageDialog *d, const char *response, gpointer selfPtr) {
       if (g_strcmp0(response, "submit") == 0) {
           GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(d), "code_entry"));
           std::string code = gtk_editable_get_text(GTK_EDITABLE(entry));
           if (!code.empty()) {
               bwp::steam::SteamService::getInstance().submit2FA(code);
           }
       }
       gtk_window_close(GTK_WINDOW(d));
  }), this);
  
  gtk_window_present(GTK_WINDOW(dialog));
}

void WorkshopView::showSteamcmdInstallDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
  auto *dialog = adw_message_dialog_new(window, "Missing steamcmd", 
    "The 'steamcmd' utility is required for Workshop downloads.\n\nPlease install it using your distribution's package manager.");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "close", "Close");
  gtk_window_present(GTK_WINDOW(dialog));
}

void WorkshopView::refreshLoginState() {
  if (!m_loginButton)
    return;
  std::string user = bwp::steam::SteamService::getInstance().getSteamUser();
  bool loggedIn = !user.empty(); 

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  if (loggedIn) {
    gtk_box_append(GTK_BOX(box),
                   gtk_image_new_from_icon_name("avatar-default-symbolic"));
    GtkWidget *label = gtk_label_new(user.c_str());
    gtk_widget_add_css_class(label, "heading");
    gtk_box_append(GTK_BOX(box), label);
    gtk_widget_add_css_class(m_loginButton, "suggested-action");
    gtk_widget_set_tooltip_text(m_loginButton, "Click to Logout");
  } else {
    gtk_box_append(GTK_BOX(box),
                   gtk_image_new_from_icon_name("system-users-symbolic"));
    gtk_box_append(GTK_BOX(box), gtk_label_new("Steam Login"));
    gtk_widget_remove_css_class(m_loginButton, "suggested-action");
    gtk_widget_set_tooltip_text(m_loginButton, "Log in to Steam");
  }
  gtk_button_set_child(GTK_BUTTON(m_loginButton), box);
}

void WorkshopView::showLogoutDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
  auto *dialog = adw_message_dialog_new(window, "Logout",
                                        "Are you sure you want to forget saved Steam user?");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "cancel",
                                  "Cancel");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "logout",
                                  "Logout");
  adw_message_dialog_set_response_appearance(
      ADW_MESSAGE_DIALOG(dialog), "logout", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(dialog), "cancel");
  g_signal_connect(dialog, "response",
                   G_CALLBACK(+[](AdwMessageDialog *d, const char *response,
                                  gpointer selfPtr) {
                     auto *self = static_cast<WorkshopView *>(selfPtr);
                     if (g_strcmp0(response, "logout") == 0) {
                       bwp::steam::SteamService::getInstance().setSteamUser("");
                       self->refreshLoginState();
                     }
                     gtk_window_close(GTK_WINDOW(d));
                   }),
                   this);
  gtk_window_present(GTK_WINDOW(dialog));
}

}
