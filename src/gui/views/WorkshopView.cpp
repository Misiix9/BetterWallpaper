#include "WorkshopView.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <iostream>

namespace bwp::gui {

WorkshopView::WorkshopView() { setupUi(); }

WorkshopView::~WorkshopView() {}

void WorkshopView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Tab Switcher
  m_stack = adw_view_stack_new();
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher),
                              ADW_VIEW_STACK(m_stack));
  adw_view_switcher_set_policy(ADW_VIEW_SWITCHER(switcher),
                               ADW_VIEW_SWITCHER_POLICY_WIDE);

  gtk_box_append(GTK_BOX(m_box), switcher);
  gtk_widget_set_vexpand(m_stack, TRUE);
  gtk_box_append(GTK_BOX(m_box), m_stack);

  setupInstalledPage();
  setupBrowsePage();

  // Defer network call to after window is shown
  // performSearch("anime"); // REMOVED - causes blocking
}

void WorkshopView::setupInstalledPage() {
  m_installedPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  m_installedGrid = std::make_unique<WallpaperGrid>();
  gtk_box_append(GTK_BOX(m_installedPage), m_installedGrid->getWidget());

  // Defer library loading to idle callback
  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<WorkshopView *>(data);
        self->refreshInstalled();
        return G_SOURCE_REMOVE;
      },
      this);

  adw_view_stack_add_titled(ADW_VIEW_STACK(m_stack), m_installedPage,
                            "installed", "Installed");
}

void WorkshopView::refreshInstalled() {
  if (!m_installedGrid)
    return;
  m_installedGrid->clear();

  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto workshopItems =
      lib.filter([](const bwp::wallpaper::WallpaperInfo &info) {
        return info.source == "workshop" ||
               info.type == bwp::wallpaper::WallpaperType::WEScene ||
               info.type == bwp::wallpaper::WallpaperType::WEVideo;
      });

  for (const auto &info : workshopItems) {
    m_installedGrid->addWallpaper(info);
  }
}

void WorkshopView::setupBrowsePage() {
  m_browsePage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(m_browsePage, 10);
  gtk_widget_set_margin_end(m_browsePage, 10);
  gtk_widget_set_margin_top(m_browsePage, 10);
  gtk_widget_set_margin_bottom(m_browsePage, 10);

  // steamcmd status banner (shows if not installed)
  m_steamcmdBanner =
      adw_banner_new("steamcmd is required for downloading Workshop content");
  adw_banner_set_button_label(ADW_BANNER(m_steamcmdBanner), "How to Install");
  adw_banner_set_revealed(ADW_BANNER(m_steamcmdBanner), false);
  g_signal_connect(m_steamcmdBanner, "button-clicked",
                   G_CALLBACK(+[](AdwBanner *, gpointer user_data) {
                     auto *self = static_cast<WorkshopView *>(user_data);
                     self->showSteamcmdInstallDialog();
                   }),
                   this);
  gtk_box_append(GTK_BOX(m_browsePage), m_steamcmdBanner);

  // Check steamcmd on load
  if (!bwp::steam::SteamWorkshopClient::getInstance().isSteamCmdAvailable()) {
    adw_banner_set_revealed(ADW_BANNER(m_steamcmdBanner), true);
  }

  // Header for Search
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(m_browsePage), headerBox);

  GtkWidget *title = gtk_label_new("Browse Steam Workshop");
  gtk_widget_add_css_class(title, "heading");
  gtk_box_append(GTK_BOX(headerBox), title);

  m_searchBar = gtk_search_entry_new();
  gtk_widget_set_hexpand(m_searchBar, TRUE);
  g_signal_connect(m_searchBar, "activate", G_CALLBACK(onSearchEnter), this);
  gtk_box_append(GTK_BOX(headerBox), m_searchBar);

  // Steam login button
  m_loginButton = gtk_button_new();
  gtk_widget_set_tooltip_text(
      m_loginButton,
      "Log in to Steam for access to more wallpapers (optional)");
  g_signal_connect(
      m_loginButton, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer user_data) {
        auto *self = static_cast<WorkshopView *>(user_data);
        if (bwp::steam::SteamWorkshopClient::getInstance().isLoggedIn()) {
          self->showLogoutDialog();
        } else {
          self->showSteamLoginDialog();
        }
      }),
      this);
  gtk_box_append(GTK_BOX(headerBox), m_loginButton);
  refreshLoginState();

  // Progress
  m_progressBar = gtk_progress_bar_new();
  gtk_box_append(GTK_BOX(m_browsePage), m_progressBar);

  // Scrolled Grid
  m_browseScrolled = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(m_browseScrolled, TRUE);
  gtk_box_append(GTK_BOX(m_browsePage), m_browseScrolled);

  m_browseGrid = gtk_flow_box_new();
  gtk_widget_set_valign(m_browseGrid, GTK_ALIGN_START);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_browseGrid), 30);
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_browseGrid), 1);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_browseGrid),
                                  GTK_SELECTION_NONE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_browseScrolled),
                                m_browseGrid);

  adw_view_stack_add_titled(ADW_VIEW_STACK(m_stack), m_browsePage, "browse",
                            "Browse");
}

void WorkshopView::onSearchEnter(GtkEntry *entry, gpointer user_data) {
  auto *self = static_cast<WorkshopView *>(user_data);
  const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
  self->performSearch(text);
}

void WorkshopView::performSearch(const std::string &query) {
  bwp::steam::SteamWorkshopClient::getInstance().search(
      query, 1, [this](const std::vector<bwp::steam::WorkshopItem> &items) {
        g_idle_add(
            [](gpointer data) -> gboolean {
              auto *pair = static_cast<std::pair<
                  WorkshopView *, std::vector<bwp::steam::WorkshopItem>> *>(
                  data);
              pair->first->updateBrowseGrid(pair->second);
              delete pair;
              return G_SOURCE_REMOVE;
            },
            new std::pair<WorkshopView *,
                          std::vector<bwp::steam::WorkshopItem>>(this, items));
      });
}

void WorkshopView::updateBrowseGrid(
    const std::vector<bwp::steam::WorkshopItem> &items) {
  GtkWidget *child = gtk_widget_get_first_child(m_browseGrid);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_flow_box_remove(GTK_FLOW_BOX(m_browseGrid), child);
    child = next;
  }

  for (const auto &item : items) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_add_css_class(card, "card");
    gtk_widget_set_size_request(card, 200, 200);

    GtkWidget *imgLabel = gtk_label_new(item.title.c_str());
    gtk_widget_set_vexpand(imgLabel, TRUE);
    gtk_box_append(GTK_BOX(card), imgLabel);

    GtkWidget *downloadBtn = gtk_button_new_with_label("Download");
    gtk_widget_set_margin_bottom(downloadBtn, 5);

    ItemData *data = new ItemData{item.id, this};
    g_object_set_data_full(
        G_OBJECT(downloadBtn), "item-data", data,
        [](gpointer d) { delete static_cast<ItemData *>(d); });

    g_signal_connect(downloadBtn, "clicked", G_CALLBACK(onDownloadClicked),
                     data);
    gtk_box_append(GTK_BOX(card), downloadBtn);

    gtk_flow_box_insert(GTK_FLOW_BOX(m_browseGrid), card, -1);
  }
}

void WorkshopView::onDownloadClicked(GtkButton * /*button*/,
                                     gpointer user_data) {
  ItemData *data = static_cast<ItemData *>(user_data);
  WorkshopView *self = data->view;

  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(self->m_progressBar));

  bwp::steam::SteamWorkshopClient::getInstance().download(
      data->id,
      [self](const bwp::steam::DownloadProgress &prog) {
        // Update progress bar from main thread
        g_idle_add(
            [](gpointer d) -> gboolean {
              auto *pair = static_cast<std::pair<WorkshopView *, double> *>(d);
              gtk_progress_bar_set_fraction(
                  GTK_PROGRESS_BAR(pair->first->m_progressBar), pair->second);
              delete pair;
              return G_SOURCE_REMOVE;
            },
            new std::pair<WorkshopView *, double>(self, prog.progress));
      },
      [self](const bwp::steam::DownloadResult &result) {
        // Create a copy of result for the idle callback
        auto *resultCopy = new bwp::steam::DownloadResult(result);

        g_idle_add(
            [](gpointer d) -> gboolean {
              auto *pair = static_cast<
                  std::pair<WorkshopView *, bwp::steam::DownloadResult *> *>(d);
              WorkshopView *view = pair->first;
              bwp::steam::DownloadResult *res = pair->second;

              gtk_progress_bar_set_fraction(
                  GTK_PROGRESS_BAR(view->m_progressBar),
                  res->error == bwp::steam::DownloadError::Success ? 1.0 : 0.0);

              if (res->error == bwp::steam::DownloadError::Success) {
                view->refreshInstalled();
              } else {
                // Show error dialog
                GtkRoot *root = gtk_widget_get_root(view->m_box);
                GtkWindow *window =
                    GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

                const char *title = "Download Failed";
                if (res->error == bwp::steam::DownloadError::SteamCmdMissing) {
                  title = "steamcmd Not Found";
                } else if (res->error ==
                           bwp::steam::DownloadError::LicenseRequired) {
                  title = "License Required";
                }

                std::string message = res->message;
                if (!res->installCmd.empty()) {
                  message += "\n\nInstall with:\n" + res->installCmd;
                }

                AdwMessageDialog *dialog = ADW_MESSAGE_DIALOG(
                    adw_message_dialog_new(window, title, message.c_str()));
                adw_message_dialog_add_response(dialog, "ok", "OK");
                adw_message_dialog_set_default_response(dialog, "ok");
                gtk_window_present(GTK_WINDOW(dialog));
              }

              delete res;
              delete pair;
              return G_SOURCE_REMOVE;
            },
            new std::pair<WorkshopView *, bwp::steam::DownloadResult *>(
                self, resultCopy));
      });
}

void WorkshopView::showSteamLoginDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

  auto *dialog = adw_message_dialog_new(
      window, "Steam Login",
      "Enter your Steam username to generate the login command.\n\n"
      "After copying, paste the command in your terminal to authenticate.\n"
      "You'll be prompted for your password and Steam Guard code.");

  // Username entry
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Steam Username");
  gtk_widget_set_margin_start(entry, 24);
  gtk_widget_set_margin_end(entry, 24);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_append(GTK_BOX(box), entry);
  adw_message_dialog_set_extra_child(ADW_MESSAGE_DIALOG(dialog), box);

  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "close", "Close");
  adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "copy",
                                  "Copy Command & Login");
  adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "copy",
                                             ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(dialog), "copy");

  g_object_set_data(G_OBJECT(dialog), "entry", entry);

  g_signal_connect(
      dialog, "response",
      G_CALLBACK(
          +[](AdwMessageDialog *d, const char *response, gpointer selfPtr) {
            auto *self = static_cast<WorkshopView *>(selfPtr);
            if (g_strcmp0(response, "copy") == 0) {
              GtkWidget *entry =
                  GTK_WIDGET(g_object_get_data(G_OBJECT(d), "entry"));
              const char *username = gtk_editable_get_text(GTK_EDITABLE(entry));

              if (username && strlen(username) > 0) {
                std::string cmd = "steamcmd +login " + std::string(username);
                GdkClipboard *clipboard =
                    gdk_display_get_clipboard(gdk_display_get_default());
                gdk_clipboard_set_text(clipboard, cmd.c_str());

                // Mark as logged in
                bwp::steam::SteamWorkshopClient::getInstance().login(username);

                // Refresh view to update login button
                self->refreshLoginState();
              }
            }
            gtk_window_close(GTK_WINDOW(d));
          }),
      this);

  gtk_window_present(GTK_WINDOW(dialog));
}

void WorkshopView::showSteamcmdInstallDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

  auto &client = bwp::steam::SteamWorkshopClient::getInstance();
  std::string distro = client.getDistroName();
  std::string installCmd = client.getInstallCommand();

  std::string message =
      "steamcmd is required to download Workshop content.\n\n";
  message += "Detected distribution: " + distro + "\n\n";
  message += "Install command:\n" + installCmd;

  AdwMessageDialog *dialog = ADW_MESSAGE_DIALOG(
      adw_message_dialog_new(window, "Install steamcmd", message.c_str()));

  adw_message_dialog_add_response(dialog, "close", "Close");
  adw_message_dialog_add_response(dialog, "copy", "Copy Command");
  adw_message_dialog_set_response_appearance(dialog, "copy",
                                             ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response(dialog, "copy");

  // Store install command for copy
  g_object_set_data_full(G_OBJECT(dialog), "install_cmd",
                         g_strdup(installCmd.c_str()), g_free);

  g_signal_connect(
      dialog, "response",
      G_CALLBACK(+[](AdwMessageDialog *d, const char *response, gpointer) {
        if (g_strcmp0(response, "copy") == 0) {
          const char *cmd =
              (const char *)g_object_get_data(G_OBJECT(d), "install_cmd");
          GdkClipboard *clipboard =
              gdk_display_get_clipboard(gdk_display_get_default());
          gdk_clipboard_set_text(clipboard, cmd);
        }
        gtk_window_close(GTK_WINDOW(d));
      }),
      nullptr);

  gtk_window_present(GTK_WINDOW(dialog));
}

void WorkshopView::refreshLoginState() {
  if (!m_loginButton)
    return;

  bool loggedIn = bwp::steam::SteamWorkshopClient::getInstance().isLoggedIn();
  std::string username =
      bwp::steam::SteamWorkshopClient::getInstance().getUsername();

  // Create content box
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

  if (loggedIn) {
    // Avatar placeholder (generic user icon)
    gtk_box_append(GTK_BOX(box),
                   gtk_image_new_from_icon_name("avatar-default-symbolic"));

    // Username
    GtkWidget *label = gtk_label_new(username.c_str());
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
                                        "Are you sure you want to log out?");

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
                       bwp::steam::SteamWorkshopClient::getInstance().logout();
                       self->refreshLoginState();
                     }
                     gtk_window_close(GTK_WINDOW(d));
                   }),
                   this);

  gtk_window_present(GTK_WINDOW(dialog));
}

} // namespace bwp::gui
