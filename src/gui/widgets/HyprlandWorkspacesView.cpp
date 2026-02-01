#include "HyprlandWorkspacesView.hpp"
#include "../../core/hyprland/HyprlandManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../dialogs/LibrarySelectionDialog.hpp"
#include <sstream>

namespace bwp::gui {

HyprlandWorkspacesView::HyprlandWorkspacesView() { setupUi(); }

HyprlandWorkspacesView::~HyprlandWorkspacesView() {}

void HyprlandWorkspacesView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(m_box, 16);
  gtk_widget_set_margin_end(m_box, 16);
  gtk_widget_set_margin_top(m_box, 16);
  gtk_widget_set_margin_bottom(m_box, 16);

  // Check if Hyprland is active
  auto &hypr = bwp::hyprland::HyprlandManager::getInstance();

  // Banner for non-Hyprland users
  m_banner = adw_banner_new("This feature is only available on Hyprland");
  adw_banner_set_revealed(ADW_BANNER(m_banner), !hypr.isActive());
  gtk_box_append(GTK_BOX(m_box), m_banner);

  // Header
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

  GtkWidget *title = gtk_label_new("Workspace Wallpapers");
  gtk_widget_add_css_class(title, "title-3");
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_widget_set_hexpand(title, TRUE);
  gtk_box_append(GTK_BOX(header), title);

  // Copy Hyprland config button
  GtkWidget *configBtn = gtk_button_new_with_label("Copy Keybinds");
  gtk_widget_set_tooltip_text(configBtn,
                              "Copy Hyprland keybind config to clipboard");
  g_signal_connect(configBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<HyprlandWorkspacesView *>(data);
                     self->showConfigSnippet();
                   }),
                   this);
  gtk_box_append(GTK_BOX(header), configBtn);

  gtk_box_append(GTK_BOX(m_box), header);

  // Description
  GtkWidget *desc = gtk_label_new(
      "Set different wallpapers for each Hyprland workspace. "
      "When you switch workspaces, the assigned wallpaper will be applied.");
  gtk_widget_add_css_class(desc, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
  gtk_label_set_xalign(GTK_LABEL(desc), 0);
  gtk_box_append(GTK_BOX(m_box), desc);

  // Scrolled list
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_append(GTK_BOX(m_box), scrolled);

  // Preferences group for workspaces
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Workspaces");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), group);
  m_workspaceList = group;

  // Add default workspaces (1-10)
  for (int i = 1; i <= 10; ++i) {
    std::string wp = hypr.getWorkspaceWallpaper(i);
    addWorkspaceRow(i, wp);
  }
}

void HyprlandWorkspacesView::addWorkspaceRow(int workspaceId,
                                             const std::string &wallpaperPath) {
  GtkWidget *row = adw_action_row_new();
  adw_preferences_row_set_title(
      ADW_PREFERENCES_ROW(row),
      ("Workspace " + std::to_string(workspaceId)).c_str());

  if (!wallpaperPath.empty()) {
    // Show filename as subtitle
    std::string filename = wallpaperPath;
    size_t pos = wallpaperPath.rfind('/');
    if (pos != std::string::npos) {
      filename = wallpaperPath.substr(pos + 1);
    }
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), filename.c_str());
  } else {
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Not set");
  }

  // Clear button
  GtkWidget *clearBtn = gtk_button_new_from_icon_name("edit-clear-symbolic");
  gtk_widget_set_valign(clearBtn, GTK_ALIGN_CENTER);
  gtk_widget_set_tooltip_text(clearBtn, "Clear wallpaper");
  gtk_widget_add_css_class(clearBtn, "flat");
  int *wsId = new int(workspaceId);
  g_object_set_data_full(G_OBJECT(clearBtn), "ws_id", wsId,
                         [](gpointer p) { delete static_cast<int *>(p); });
  g_signal_connect(
      clearBtn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
        auto *self = static_cast<HyprlandWorkspacesView *>(data);
        int wsId =
            *static_cast<int *>(g_object_get_data(G_OBJECT(btn), "ws_id"));
        bwp::hyprland::HyprlandManager::getInstance().setWorkspaceWallpaper(
            wsId, "");
        self->refresh();
      }),
      this);
  adw_action_row_add_suffix(ADW_ACTION_ROW(row), clearBtn);

  // Browse button
  GtkWidget *browseBtn =
      gtk_button_new_from_icon_name("folder-pictures-symbolic");
  gtk_widget_set_valign(browseBtn, GTK_ALIGN_CENTER);
  gtk_widget_set_tooltip_text(browseBtn, "Select wallpaper");
  gtk_widget_add_css_class(browseBtn, "flat");
  int *wsId2 = new int(workspaceId);
  g_object_set_data_full(G_OBJECT(browseBtn), "ws_id", wsId2,
                         [](gpointer p) { delete static_cast<int *>(p); });
  g_object_set_data(G_OBJECT(browseBtn), "row", row);
  g_signal_connect(
      browseBtn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
        auto *self = static_cast<HyprlandWorkspacesView *>(data);
        int wsId =
            *static_cast<int *>(g_object_get_data(G_OBJECT(btn), "ws_id"));
        GtkWidget *row = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "row"));
        self->onSelectWallpaper(wsId, row);
      }),
      this);
  adw_action_row_add_suffix(ADW_ACTION_ROW(row), browseBtn);

  adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), browseBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_workspaceList), row);
  m_workspaceRows[workspaceId] = row;
}

void HyprlandWorkspacesView::onSelectWallpaper(int workspaceId,
                                               GtkWidget *row) {
  GtkWidget *topLevel = gtk_widget_get_ancestor(m_box, GTK_TYPE_WINDOW);
  GtkWindow *window = GTK_IS_WINDOW(topLevel) ? GTK_WINDOW(topLevel) : nullptr;

  auto *dialog = new LibrarySelectionDialog(window);

  // Need to capture row and workspace ID for callback
  // row might be destroyed if view is refreshed, but refreshed only happens on
  // changes? Ideally refresh shouldn't destroy rows if we only update them. But
  // refresh() in header iterates rows.

  // Use a weak reference or just assume view stays alive while dialog is open.
  // The dialog blocks interaction if modal.

  dialog->show([this, workspaceId, row](const std::string &path) {
    bwp::hyprland::HyprlandManager::getInstance().setWorkspaceWallpaper(
        workspaceId, path);

    // Update row subtitle
    std::string filename = path;
    size_t pos = filename.rfind('/');
    if (pos != std::string::npos) {
      filename = filename.substr(pos + 1);
    }
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), filename.c_str());
  });
}

void HyprlandWorkspacesView::showConfigSnippet() {
  auto &hypr = bwp::hyprland::HyprlandManager::getInstance();
  std::string snippet = hypr.generateConfigSnippet();

  GtkWidget *topLevel = gtk_widget_get_ancestor(m_box, GTK_TYPE_WINDOW);
  GtkWindow *window = GTK_IS_WINDOW(topLevel) ? GTK_WINDOW(topLevel) : nullptr;

  AdwMessageDialog *dialog = ADW_MESSAGE_DIALOG(
      adw_message_dialog_new(window, "Hyprland Keybinds", snippet.c_str()));
  adw_message_dialog_add_response(dialog, "close", "Close");
  adw_message_dialog_add_response(dialog, "copy", "Copy to Clipboard");
  adw_message_dialog_set_response_appearance(dialog, "copy",
                                             ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response(dialog, "copy");

  g_object_set_data_full(G_OBJECT(dialog), "snippet", g_strdup(snippet.c_str()),
                         g_free);

  g_signal_connect(
      dialog, "response",
      G_CALLBACK(+[](AdwMessageDialog *d, const char *response, gpointer) {
        if (g_strcmp0(response, "copy") == 0) {
          const char *text =
              (const char *)g_object_get_data(G_OBJECT(d), "snippet");
          GdkClipboard *clipboard =
              gdk_display_get_clipboard(gdk_display_get_default());
          gdk_clipboard_set_text(clipboard, text);
        }
        gtk_window_close(GTK_WINDOW(d));
      }),
      nullptr);

  gtk_window_present(GTK_WINDOW(dialog));
}

void HyprlandWorkspacesView::refresh() {
  auto &hypr = bwp::hyprland::HyprlandManager::getInstance();

  // Update banner visibility
  adw_banner_set_revealed(ADW_BANNER(m_banner), !hypr.isActive());

  // Update workspace rows
  for (auto &[wsId, row] : m_workspaceRows) {
    std::string wp = hypr.getWorkspaceWallpaper(wsId);
    if (!wp.empty()) {
      std::string filename = wp;
      size_t pos = wp.rfind('/');
      if (pos != std::string::npos) {
        filename = wp.substr(pos + 1);
      }
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), filename.c_str());
    } else {
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Not set");
    }
  }
}

} // namespace bwp::gui
