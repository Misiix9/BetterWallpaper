#include "SettingsView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/system/AutostartManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/utils/ToastManager.hpp"
#include "../../core/wallpaper/LibraryScanner.hpp"
#include "../../core/wallpaper/WallpaperManager.hpp"
#include <algorithm>
#include <iostream>
namespace bwp::gui {
SettingsView::SettingsView() {
  setupUi();
  takeSnapshot();
  bwp::config::ConfigManager::getInstance().startWatching(
      [this](const std::string &, const nlohmann::json &) {
        if (!m_hasUnsavedChanges) {
          m_hasUnsavedChanges = true;
          g_idle_add(
              +[](gpointer data) -> gboolean {
                auto *self = static_cast<SettingsView *>(data);
                self->showUnsavedBar();
                return G_SOURCE_REMOVE;
              },
              this);
        }
      });
}
SettingsView::~SettingsView() {
  bwp::config::ConfigManager::getInstance().startWatching(nullptr);
}
void SettingsView::takeSnapshot() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  m_configSnapshot = conf.getJson();
  m_hasUnsavedChanges = false;
}
void SettingsView::showUnsavedBar() {
  if (m_unsavedRevealer) {
    gtk_revealer_set_reveal_child(GTK_REVEALER(m_unsavedRevealer), TRUE);
  }
}
void SettingsView::hideUnsavedBar() {
  if (m_unsavedRevealer) {
    gtk_revealer_set_reveal_child(GTK_REVEALER(m_unsavedRevealer), FALSE);
  }
}
void SettingsView::onKeepChanges() {
  takeSnapshot();
  hideUnsavedBar();
  bwp::core::utils::ToastManager::getInstance().showSuccess("Settings saved");
}
void SettingsView::onDiscardChanges() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  conf.startWatching(nullptr);
  conf.getJson() = m_configSnapshot;
  m_hasUnsavedChanges = false;
  hideUnsavedBar();
  rebuildPages();
  conf.startWatching([this](const std::string &, const nlohmann::json &) {
    if (!m_hasUnsavedChanges) {
      m_hasUnsavedChanges = true;
      g_idle_add(
          +[](gpointer data) -> gboolean {
            auto *self = static_cast<SettingsView *>(data);
            self->showUnsavedBar();
            return G_SOURCE_REMOVE;
          },
          this);
    }
  });
  bwp::core::utils::ToastManager::getInstance().showInfo("Changes discarded");
}
void SettingsView::rebuildPages() {
  m_pageInfos.clear();
  while (true) {
    GtkListBoxRow *row =
        gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_sidebarList), 0);
    if (!row)
      break;
    gtk_list_box_remove(GTK_LIST_BOX(m_sidebarList), GTK_WIDGET(row));
  }
#if ADW_CHECK_VERSION(1, 4, 0)
  while (gtk_widget_get_first_child(m_stack)) {
    adw_view_stack_remove(ADW_VIEW_STACK(m_stack),
                          gtk_widget_get_first_child(m_stack));
  }
#else
  while (true) {
    GtkWidget *child = gtk_widget_get_first_child(m_stack);
    if (!child)
      break;
    gtk_stack_remove(GTK_STACK(m_stack), child);
  }
#endif
  m_transitionEffectDropdown = nullptr;
  m_transitionDurationSpin = nullptr;
  m_transitionEasingDropdown = nullptr;
  m_transitionEnabledSwitch = nullptr;
  m_currentLibraryGroup = nullptr;
  auto addPage = [&](const char *id, const char *title, const char *icon,
                     GtkWidget *pageWidget,
                     std::vector<std::string> keywords = {}) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_stack), pageWidget, id);
#else
    gtk_stack_add_named(GTK_STACK(m_stack), pageWidget, id);
#endif
    keywords.push_back(title);
    keywords.push_back(id);
    m_pageInfos.push_back({id, title, keywords});
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_box_append(GTK_BOX(box), gtk_image_new_from_icon_name(icon));
    gtk_box_append(GTK_BOX(box), gtk_label_new(title));
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    g_object_set_data_full(G_OBJECT(row), "page_id", g_strdup(id), g_free);
    gtk_list_box_append(GTK_LIST_BOX(m_sidebarList), row);
  };
  addPage("general", "General", "emblem-system-symbolic", createGeneralPage(),
          {"startup", "autostart", "theme", "language", "notification"});
  addPage("graphics", "Graphics", "video-display-symbolic",
          createGraphicsPage(),
          {"fps", "framerate", "resolution", "rendering", "gpu"});
  addPage("audio", "Audio", "audio-volume-high-symbolic", createAudioPage(),
          {"volume", "mute", "sound", "speaker"});
  addPage("playback", "Playback", "media-playback-start-symbolic",
          createPlaybackPage(),
          {"speed", "loop", "shuffle", "slideshow", "interval"});
  addPage("transitions", "Transitions", "view-refresh-symbolic",
          createTransitionsPage(),
          {"fade", "slide", "effect", "animation", "duration", "easing"});
  addPage("controls", "Controls", "input-keyboard-symbolic",
          createControlsPage(),
          {"keybind", "shortcut", "hotkey", "keyboard", "mouse"});
  addPage("sources", "Sources", "folder-pictures-symbolic", createSourcesPage(),
          {"folder", "directory", "library", "path", "steam", "workshop"});
  addPage("about", "About", "help-about-symbolic", createAboutPage(),
          {"version", "license", "credits", "author"});
  GtkListBoxRow *first =
      gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_sidebarList), 0);
  if (first)
    gtk_list_box_select_row(GTK_LIST_BOX(m_sidebarList), first);
}
void SettingsView::setupUi() {
  m_outerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#if ADW_CHECK_VERSION(1, 4, 0)
  m_splitView = adw_overlay_split_view_new();
#else
  m_splitView = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#endif
  m_content = m_splitView;
  gtk_widget_set_vexpand(m_splitView, TRUE);
  gtk_box_append(GTK_BOX(m_outerBox), m_splitView);
  m_unsavedRevealer = gtk_revealer_new();
  gtk_revealer_set_transition_type(GTK_REVEALER(m_unsavedRevealer),
                                   GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  gtk_revealer_set_transition_duration(GTK_REVEALER(m_unsavedRevealer), 200);
  gtk_revealer_set_reveal_child(GTK_REVEALER(m_unsavedRevealer), FALSE);
  GtkWidget *barBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_add_css_class(barBox, "toolbar");
  gtk_widget_set_margin_start(barBox, 16);
  gtk_widget_set_margin_end(barBox, 16);
  gtk_widget_set_margin_top(barBox, 8);
  gtk_widget_set_margin_bottom(barBox, 8);
  GtkWidget *barIcon = gtk_image_new_from_icon_name("document-edit-symbolic");
  gtk_box_append(GTK_BOX(barBox), barIcon);
  GtkWidget *barLabel = gtk_label_new("You have unsaved changes");
  gtk_widget_set_hexpand(barLabel, TRUE);
  gtk_label_set_xalign(GTK_LABEL(barLabel), 0.0);
  gtk_box_append(GTK_BOX(barBox), barLabel);
  GtkWidget *discardBtn = gtk_button_new_with_label("Discard");
  gtk_widget_add_css_class(discardBtn, "destructive-action");
  g_signal_connect(discardBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<SettingsView *>(data)->onDiscardChanges();
                   }),
                   this);
  gtk_box_append(GTK_BOX(barBox), discardBtn);
  GtkWidget *saveBtn = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(saveBtn, "suggested-action");
  g_signal_connect(saveBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<SettingsView *>(data)->onKeepChanges();
                   }),
                   this);
  gtk_box_append(GTK_BOX(barBox), saveBtn);
  gtk_revealer_set_child(GTK_REVEALER(m_unsavedRevealer), barBox);
  gtk_box_append(GTK_BOX(m_outerBox), m_unsavedRevealer);
  GtkWidget *sidebarBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(sidebarBox, 200, -1);
  gtk_widget_add_css_class(sidebarBox, "background");
  GtkWidget *header = gtk_label_new("Settings");
  gtk_widget_add_css_class(header, "title-2");
  gtk_widget_set_margin_top(header, 12);
  gtk_widget_set_margin_bottom(header, 8);
  gtk_box_append(GTK_BOX(sidebarBox), header);
  m_searchEntry = gtk_search_entry_new();
  gtk_widget_set_margin_start(m_searchEntry, 8);
  gtk_widget_set_margin_end(m_searchEntry, 8);
  gtk_widget_set_margin_bottom(m_searchEntry, 8);
  g_signal_connect(m_searchEntry, "search-changed",
                   G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<SettingsView *>(data);
                     const char *text =
                         gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->filterSettings(text ? text : "");
                   }),
                   this);
  gtk_box_append(GTK_BOX(sidebarBox), m_searchEntry);
  m_sidebarList = gtk_list_box_new();
  gtk_widget_add_css_class(m_sidebarList, "navigation-sidebar");
  gtk_widget_set_vexpand(m_sidebarList, TRUE);
  gtk_box_append(GTK_BOX(sidebarBox), m_sidebarList);
#if ADW_CHECK_VERSION(1, 4, 0)
  adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(m_splitView),
                                     sidebarBox);
#else
  gtk_box_append(GTK_BOX(m_splitView), sidebarBox);
  gtk_box_append(GTK_BOX(m_splitView),
                 gtk_separator_new(GTK_ORIENTATION_VERTICAL));
#endif
#if ADW_CHECK_VERSION(1, 4, 0)
  m_stack = adw_view_stack_new();
  adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(m_splitView),
                                     m_stack);
#else
  m_stack = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(m_stack),
                                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_widget_set_hexpand(m_stack, TRUE);
  gtk_box_append(GTK_BOX(m_splitView), m_stack);
#endif
  auto addPage = [&](const char *id, const char *title, const char *icon,
                     GtkWidget *pageWidget,
                     std::vector<std::string> keywords = {}) {
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_stack), pageWidget, id);
#else
    gtk_stack_add_named(GTK_STACK(m_stack), pageWidget, id);
#endif
    keywords.push_back(title);
    keywords.push_back(id);
    m_pageInfos.push_back({id, title, keywords});
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_box_append(GTK_BOX(box), gtk_image_new_from_icon_name(icon));
    gtk_box_append(GTK_BOX(box), gtk_label_new(title));
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    g_object_set_data_full(G_OBJECT(row), "page_id", g_strdup(id), g_free);
    gtk_list_box_append(GTK_LIST_BOX(m_sidebarList), row);
  };
  addPage("general", "General", "emblem-system-symbolic", createGeneralPage(),
          {"startup", "autostart", "theme", "language", "notification"});
  addPage("graphics", "Graphics", "video-display-symbolic",
          createGraphicsPage(),
          {"fps", "framerate", "resolution", "rendering", "gpu"});
  addPage("audio", "Audio", "audio-volume-high-symbolic", createAudioPage(),
          {"volume", "mute", "sound", "speaker"});
  addPage("playback", "Playback", "media-playback-start-symbolic",
          createPlaybackPage(),
          {"speed", "loop", "shuffle", "slideshow", "interval"});
  addPage("transitions", "Transitions", "view-refresh-symbolic",
          createTransitionsPage(),
          {"fade", "slide", "effect", "animation", "duration", "easing"});
  addPage("controls", "Controls", "input-keyboard-symbolic",
          createControlsPage(),
          {"keybind", "shortcut", "hotkey", "keyboard", "mouse"});
  addPage("sources", "Sources", "folder-pictures-symbolic", createSourcesPage(),
          {"folder", "directory", "library", "path", "steam", "workshop"});
  addPage("about", "About", "help-about-symbolic", createAboutPage(),
          {"version", "license", "credits", "author"});
  g_signal_connect(
      m_sidebarList, "row-activated",
      G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        SettingsView *self = static_cast<SettingsView *>(data);
        const char *id =
            (const char *)g_object_get_data(G_OBJECT(row), "page_id");
        if (id) {
#if ADW_CHECK_VERSION(1, 4, 0)
          adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(self->m_stack),
                                                id);
#else
          gtk_stack_set_visible_child_name(GTK_STACK(self->m_stack), id);
#endif
        }
      }),
      this);
  GtkListBoxRow *first =
      gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_sidebarList), 0);
  if (first)
    gtk_list_box_select_row(GTK_LIST_BOX(m_sidebarList), first);
}
void SettingsView::filterSettings(const std::string &query) {
  std::string lower = query;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  for (int i = 0; i < static_cast<int>(m_pageInfos.size()); ++i) {
    GtkListBoxRow *row =
        gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_sidebarList), i);
    if (!row)
      continue;
    if (lower.empty()) {
      gtk_widget_set_visible(GTK_WIDGET(row), TRUE);
      continue;
    }
    bool match = false;
    for (const auto &kw : m_pageInfos[i].keywords) {
      std::string kwLower = kw;
      std::transform(kwLower.begin(), kwLower.end(), kwLower.begin(),
                     ::tolower);
      if (kwLower.find(lower) != std::string::npos) {
        match = true;
        break;
      }
    }
    gtk_widget_set_visible(GTK_WIDGET(row), match);
    if (match) {
      const char *id =
          (const char *)g_object_get_data(G_OBJECT(row), "page_id");
      if (id) {
#if ADW_CHECK_VERSION(1, 4, 0)
        adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_stack), id);
#else
        gtk_stack_set_visible_child_name(GTK_STACK(m_stack), id);
#endif
      }
    }
  }
}
GtkWidget *SettingsView::createSourcesPage() {
  GtkWidget *page = adw_preferences_page_new();
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group),
                                  "Library Sources");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(group), "Manage folders scanned for wallpapers");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(group));
  m_currentLibraryGroup = group;
  updateLibraryList(group);
  return page;
}
void SettingsView::setupLibraryList(GtkWidget *group) {
  updateLibraryList(group);
}
void SettingsView::updateLibraryList(GtkWidget *group) {
  GtkWidget *child = gtk_widget_get_first_child(group);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    adw_preferences_group_remove(ADW_PREFERENCES_GROUP(group), child);
    child = next;
  }
  GtkWidget *addRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(addRow), "Add New Source");
  GtkWidget *addBtn = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_set_valign(addBtn, GTK_ALIGN_CENTER);
  g_object_set_data(G_OBJECT(addBtn), "group_ptr", group);
  g_signal_connect(
      addBtn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
        SettingsView *self = static_cast<SettingsView *>(data);
        GtkWidget *grp =
            (GtkWidget *)g_object_get_data(G_OBJECT(btn), "group_ptr");
        self->onAddSource(grp);
      }),
      this);
  adw_action_row_add_suffix(ADW_ACTION_ROW(addRow), addBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), addRow);
  auto paths =
      bwp::config::ConfigManager::getInstance().get<std::vector<std::string>>(
          "library.paths");
  for (const auto &path : paths) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), path.c_str());
    GtkWidget *delBtn = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_set_valign(delBtn, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(delBtn, "destructive-action");
    std::string *pathCopy = new std::string(path);
    g_object_set_data_full(
        G_OBJECT(delBtn), "path", pathCopy,
        [](gpointer data) { delete static_cast<std::string *>(data); });
    g_object_set_data(G_OBJECT(delBtn), "group_ptr", group);
    g_signal_connect(
        delBtn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
          SettingsView *self = static_cast<SettingsView *>(data);
          std::string *p = static_cast<std::string *>(
              g_object_get_data(G_OBJECT(btn), "path"));
          GtkWidget *grp =
              (GtkWidget *)g_object_get_data(G_OBJECT(btn), "group_ptr");
          if (p && grp)
            self->onRemoveSource(grp, *p);
        }),
        this);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), delBtn);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), row);
  }
}
void SettingsView::onAddSource(GtkWidget *group) {
  auto dialog = gtk_file_chooser_native_new(
      "Select Wallpaper Folder", GTK_WINDOW(gtk_widget_get_root(m_content)),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "Select", "Cancel");
  g_object_set_data(G_OBJECT(dialog), "group_ptr", group);
  g_signal_connect(
      dialog, "response",
      G_CALLBACK(+[](GtkNativeDialog *dialog, int response, gpointer data) {
        SettingsView *self = static_cast<SettingsView *>(data);
        GtkWidget *grp =
            (GtkWidget *)g_object_get_data(G_OBJECT(dialog), "group_ptr");
        if (response == GTK_RESPONSE_ACCEPT) {
          GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
          char *path = g_file_get_path(file);
          if (path) {
            auto &conf = bwp::config::ConfigManager::getInstance();
            auto paths = conf.get<std::vector<std::string>>("library.paths");
            bool exists = false;
            for (const auto &p : paths)
              if (p == path)
                exists = true;
            if (!exists) {
              paths.push_back(path);
              conf.set("library.paths", paths);
              if (grp)
                self->updateLibraryList(grp);
              bwp::wallpaper::LibraryScanner::getInstance().scan(paths);
            }
            g_free(path);
          }
          g_object_unref(file);
        }
        g_object_unref(dialog);
      }),
      this);
  gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}
void SettingsView::onRemoveSource(GtkWidget *group, const std::string &path) {
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto paths = conf.get<std::vector<std::string>>("library.paths");
  auto it = std::remove(paths.begin(), paths.end(), path);
  if (it != paths.end()) {
    paths.erase(it, paths.end());
    conf.set("library.paths", paths);
    updateLibraryList(group);
    bwp::wallpaper::LibraryScanner::getInstance().scan(paths);
  }
}
GtkWidget *SettingsView::createGeneralPage() {
  GtkWidget *page = adw_preferences_page_new();
  GtkWidget *appGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(appGroup),
                                  "Application");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(appGroup));
  auto &conf = bwp::config::ConfigManager::getInstance();
  GtkWidget *autostartRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(autostartRow),
                                "Start on Boot");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(autostartRow),
                              "Launch automatically");
  adw_switch_row_set_active(ADW_SWITCH_ROW(autostartRow),
                            conf.get<bool>("general.autostart"));
  g_signal_connect(autostartRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     bool active = adw_switch_row_get_active(row);
                     bwp::config::ConfigManager::getInstance().set(
                         "general.autostart", active);
                     if (active) {
                       bwp::core::AutostartManager::getInstance().enable(
                           bwp::core::AutostartMethod::XDGAutostart);
                     } else {
                       bwp::core::AutostartManager::getInstance().disable();
                     }
                     bwp::core::utils::ToastManager::getInstance().showToast(
                         "Settings saved");
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(appGroup), autostartRow);
  GtkWidget *trayRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(trayRow),
                                "Start Minimized");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(trayRow), "Start hidden in tray");
  adw_switch_row_set_active(ADW_SWITCH_ROW(trayRow),
                            conf.get<bool>("general.start_minimized"));
  g_signal_connect(trayRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "general.start_minimized",
                         static_cast<bool>(adw_switch_row_get_active(row)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(appGroup), trayRow);
  GtkWidget *windowModeRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(windowModeRow),
                                "Window Mode");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(windowModeRow),
                              "Restart required");
  const char *modes[] = {"Tiling (Resizable)", "Floating (Fixed Size)", NULL};
  GtkStringList *modeModel = gtk_string_list_new(modes);
  adw_combo_row_set_model(ADW_COMBO_ROW(windowModeRow),
                          G_LIST_MODEL(modeModel));
  g_object_unref(modeModel);
  std::string currentMode = conf.get<std::string>("general.window_mode");
  adw_combo_row_set_selected(ADW_COMBO_ROW(windowModeRow),
                             (currentMode == "floating") ? 1 : 0);
  g_signal_connect(windowModeRow, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     int idx = adw_combo_row_get_selected(row);
                     bwp::config::ConfigManager::getInstance().set(
                         "general.window_mode",
                         (idx == 1) ? "floating" : "tiling");
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(appGroup), windowModeRow);
  GtkWidget *themeGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(themeGroup),
                                  "Appearance & Theming");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(themeGroup));
  GtkWidget *extractRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(extractRow),
                                "Color Extraction");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(extractRow),
                              "Generate system theme from wallpaper");
  adw_switch_row_set_active(ADW_SWITCH_ROW(extractRow),
                            conf.get<bool>("theming.enabled"));
  g_signal_connect(extractRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "theming.enabled",
                         static_cast<bool>(adw_switch_row_get_active(row)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(themeGroup), extractRow);
  GtkWidget *toolRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(toolRow),
                                "Theming Backend");
  const char *tools[] = {"PyWal", "Matugen", "Custom Script", NULL};
  GtkStringList *toolModel = gtk_string_list_new(tools);
  adw_combo_row_set_model(ADW_COMBO_ROW(toolRow), G_LIST_MODEL(toolModel));
  g_object_unref(toolModel);
  std::string tool = conf.get<std::string>("theming.tool");
  int toolIdx = 0;
  if (tool == "matugen")
    toolIdx = 1;
  else if (tool == "custom")
    toolIdx = 2;
  adw_combo_row_set_selected(ADW_COMBO_ROW(toolRow), toolIdx);
  g_signal_connect(toolRow, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     int idx = adw_combo_row_get_selected(row);
                     std::string t = "pywal";
                     if (idx == 1)
                       t = "matugen";
                     else if (idx == 2)
                       t = "custom";
                     bwp::config::ConfigManager::getInstance().set(
                         "theming.tool", t);
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(themeGroup), toolRow);
  GtkWidget *backupGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(backupGroup),
                                  "Backup &amp; Reset");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(backupGroup));
  GtkWidget *exportRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(exportRow),
                                "Export Configuration");
  GtkWidget *exportBtn =
      gtk_button_new_from_icon_name("document-save-symbolic");
  gtk_widget_set_valign(exportBtn, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix(ADW_ACTION_ROW(exportRow), exportBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(backupGroup), exportRow);
  GtkWidget *resetRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(resetRow),
                                "Reset to Defaults");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(resetRow),
                              "Restore all settings to their original values");
  GtkWidget *resetBtn = gtk_button_new_with_label("Reset");
  gtk_widget_add_css_class(resetBtn, "destructive-action");
  gtk_widget_set_valign(resetBtn, GTK_ALIGN_CENTER);
  g_signal_connect(
      resetBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer) {
        bwp::config::ConfigManager::getInstance().resetToDefaults();
        bwp::core::utils::ToastManager::getInstance().showToast(
            "All settings reset to defaults. Restart recommended.");
      }),
      nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(resetRow), resetBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(backupGroup), resetRow);
  return page;
}
GtkWidget *SettingsView::createGraphicsPage() {
  GtkWidget *page = adw_preferences_page_new();
  auto &conf = bwp::config::ConfigManager::getInstance();
  GtkWidget *perfGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(perfGroup),
                                  "Performance");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(perfGroup));
  GtkWidget *fpsRow = adw_spin_row_new_with_range(1, 240, 1);
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(fpsRow),
                                "Global FPS Limit");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(fpsRow),
                              "Maximum frames per second for wallpapers");
  int currentFps = conf.get<int>("performance.fps_limit");
  if (currentFps <= 0)
    currentFps = 60;
  adw_spin_row_set_value(ADW_SPIN_ROW(fpsRow), currentFps);
  g_signal_connect(
      fpsRow, "notify::value",
      G_CALLBACK(+[](AdwSpinRow *row, GParamSpec *, gpointer) {
        int fps = static_cast<int>(adw_spin_row_get_value(row));
        bwp::config::ConfigManager::getInstance().set("performance.fps_limit",
                                                      fps);
        // Apply live to active wallpapers
        bwp::wallpaper::WallpaperManager::getInstance().setFpsLimit(fps);
      }),
      nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(perfGroup), fpsRow);
  GtkWidget *scalingRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scalingRow),
                                "Default Scaling");
  const char *s_opts[] = {"Fill", "Fit",  "Stretch", "Center",
                          "Tile", "Zoom", NULL};
  GtkStringList *s_model = gtk_string_list_new(s_opts);
  adw_combo_row_set_model(ADW_COMBO_ROW(scalingRow), G_LIST_MODEL(s_model));
  g_object_unref(s_model);
  std::string s_mode = conf.get<std::string>("defaults.scaling_mode");
  int s_idx = 0;
  if (s_mode == "fit")
    s_idx = 1;
  else if (s_mode == "stretch")
    s_idx = 2;
  else if (s_mode == "center")
    s_idx = 3;
  else if (s_mode == "tile")
    s_idx = 4;
  else if (s_mode == "zoom")
    s_idx = 5;
  adw_combo_row_set_selected(ADW_COMBO_ROW(scalingRow), s_idx);
  g_signal_connect(scalingRow, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     int idx = adw_combo_row_get_selected(row);
                     const char *v = "fill";
                     switch (idx) {
                     case 1:
                       v = "fit";
                       break;
                     case 2:
                       v = "stretch";
                       break;
                     case 3:
                       v = "center";
                       break;
                     case 4:
                       v = "tile";
                       break;
                     case 5:
                       v = "zoom";
                       break;
                     default:
                       v = "fill";
                       break;
                     }
                     bwp::config::ConfigManager::getInstance().set(
                         "defaults.scaling_mode", std::string(v));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(perfGroup), scalingRow);
  GtkWidget *resourceGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(resourceGroup),
                                  "Resource Management");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(resourceGroup),
      "Limit memory usage to prevent system slowdown");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(resourceGroup));
  GtkWidget *ramRow = adw_spin_row_new_with_range(512, 8192, 256);
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ramRow), "RAM Limit (MB)");
  adw_action_row_set_subtitle(
      ADW_ACTION_ROW(ramRow),
      "Wallpaper pauses when memory exceeds this limit");
  int currentRam = conf.get<int>("performance.ram_limit_mb");
  if (currentRam <= 0)
    currentRam = 2048;
  adw_spin_row_set_value(ADW_SPIN_ROW(ramRow), currentRam);
  g_signal_connect(ramRow, "notify::value",
                   G_CALLBACK(+[](AdwSpinRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "performance.ram_limit_mb",
                         (int)adw_spin_row_get_value(r));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(resourceGroup), ramRow);
  GtkWidget *gpuRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gpuRow), "GPU Preference");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(gpuRow),
                              "Auto: iGPU for battery, dGPU for heavy scenes");
  const char *gpu_opts[] = {"Auto", "Integrated (iGPU)", "Dedicated (dGPU)",
                            NULL};
  GtkStringList *gpu_model = gtk_string_list_new(gpu_opts);
  adw_combo_row_set_model(ADW_COMBO_ROW(gpuRow), G_LIST_MODEL(gpu_model));
  g_object_unref(gpu_model);
  std::string gpuPref = conf.get<std::string>("performance.gpu_preference");
  int gpuIdx = 0;
  if (gpuPref == "igpu")
    gpuIdx = 1;
  else if (gpuPref == "dgpu")
    gpuIdx = 2;
  adw_combo_row_set_selected(ADW_COMBO_ROW(gpuRow), gpuIdx);
  g_signal_connect(gpuRow, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     int idx = adw_combo_row_get_selected(row);
                     const char *v = "auto";
                     if (idx == 1)
                       v = "igpu";
                     else if (idx == 2)
                       v = "dgpu";
                     bwp::config::ConfigManager::getInstance().set(
                         "performance.gpu_preference", std::string(v));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(resourceGroup), gpuRow);
  GtkWidget *contentGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(contentGroup),
                                  "Content");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(contentGroup));
  GtkWidget *nsfwRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(nsfwRow),
                                "Show Adult Content");
  adw_action_row_set_subtitle(
      ADW_ACTION_ROW(nsfwRow),
      "Display NSFW wallpapers in Workshop search results");
  adw_switch_row_set_active(ADW_SWITCH_ROW(nsfwRow),
                            conf.get<bool>("content.show_nsfw"));
  g_signal_connect(nsfwRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "content.show_nsfw",
                         static_cast<bool>(adw_switch_row_get_active(r)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(contentGroup), nsfwRow);
  GtkWidget *slideGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(slideGroup),
                                  "Slideshow");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(slideGroup));
  GtkWidget *intervalRow = adw_spin_row_new_with_range(5, 3600, 5);
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(intervalRow),
                                "Interval (seconds)");
  adw_spin_row_set_value(ADW_SPIN_ROW(intervalRow),
                         conf.get<int>("slideshow.interval"));
  g_signal_connect(intervalRow, "notify::value",
                   G_CALLBACK(+[](AdwSpinRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "slideshow.interval", (int)adw_spin_row_get_value(r));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(slideGroup), intervalRow);
  GtkWidget *shuffleRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(shuffleRow), "Shuffle");
  adw_switch_row_set_active(ADW_SWITCH_ROW(shuffleRow),
                            conf.get<bool>("slideshow.shuffle"));
  g_signal_connect(shuffleRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "slideshow.shuffle",
                         static_cast<bool>(adw_switch_row_get_active(r)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(slideGroup), shuffleRow);
  return page;
}
GtkWidget *SettingsView::createAudioPage() {
  GtkWidget *page = adw_preferences_page_new();
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group),
                                  "Audio Settings");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(group));
  auto &conf = bwp::config::ConfigManager::getInstance();
  GtkWidget *volRow = adw_spin_row_new_with_range(0, 100, 1);
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(volRow),
                                "Global Volume Target");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(volRow),
                              "Default volume for new wallpapers");
  adw_spin_row_set_value(ADW_SPIN_ROW(volRow),
                         conf.get<int>("defaults.audio_volume"));
  g_signal_connect(
      volRow, "notify::value",
      G_CALLBACK(+[](AdwSpinRow *r, GParamSpec *, gpointer) {
        int vol = (int)adw_spin_row_get_value(r);
        bwp::config::ConfigManager::getInstance().set("defaults.audio_volume",
                                                      vol);
        // Apply live to active wallpapers
        bwp::wallpaper::WallpaperManager::getInstance().setVolumeLevel(vol);
      }),
      nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), volRow);
  GtkWidget *muteRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(muteRow), "Audio Enabled");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(muteRow),
                              "If disabled, all wallpapers will be muted");
  adw_switch_row_set_active(ADW_SWITCH_ROW(muteRow),
                            conf.get<bool>("defaults.audio_enabled"));
  g_signal_connect(
      muteRow, "notify::active",
      G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
        bool enabled = static_cast<bool>(adw_switch_row_get_active(r));
        bwp::config::ConfigManager::getInstance().set("defaults.audio_enabled",
                                                      enabled);
        // Apply live to active wallpapers — muted is the inverse of
        // audio_enabled
        bwp::wallpaper::WallpaperManager::getInstance().setMuted(!enabled);
      }),
      nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), muteRow);
  return page;
}
GtkWidget *SettingsView::createPlaybackPage() {
  GtkWidget *page = adw_preferences_page_new();
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Power Saving");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(group));
  auto &conf = bwp::config::ConfigManager::getInstance();
  GtkWidget *batRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(batRow),
                                "Pause on Battery");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(batRow),
                              "Pause wallpaper when discharging");
  adw_switch_row_set_active(ADW_SWITCH_ROW(batRow),
                            conf.get<bool>("performance.pause_on_battery"));
  g_signal_connect(batRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "performance.pause_on_battery",
                         static_cast<bool>(adw_switch_row_get_active(r)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), batRow);

  // --- Wallpaper Engine Defaults ---
  GtkWidget *weGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(weGroup),
                                  "Wallpaper Engine Defaults");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(weGroup),
      "Default behavior for Wallpaper Engine scenes");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(weGroup));

  GtkWidget *mouseRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mouseRow),
                                "Disable Mouse Interaction");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(mouseRow),
                              "Prevent mouse events from reaching wallpaper");
  adw_switch_row_set_active(ADW_SWITCH_ROW(mouseRow),
                            conf.get<bool>("defaults.disable_mouse", true));
  g_signal_connect(
      mouseRow, "notify::active",
      G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
        bool val = static_cast<bool>(adw_switch_row_get_active(r));
        bwp::config::ConfigManager::getInstance().set("defaults.disable_mouse",
                                                      val);
        bwp::wallpaper::WallpaperManager::getInstance().setDisableMouse(val);
      }),
      nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(weGroup), mouseRow);

  GtkWidget *noAudioProcRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(noAudioProcRow),
                                "Disable Audio Processing");
  adw_action_row_set_subtitle(
      ADW_ACTION_ROW(noAudioProcRow),
      "Skip audio processing for wallpaper engine scenes");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(noAudioProcRow),
      conf.get<bool>("defaults.no_audio_processing", true));
  g_signal_connect(
      noAudioProcRow, "notify::active",
      G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
        bool val = static_cast<bool>(adw_switch_row_get_active(r));
        bwp::config::ConfigManager::getInstance().set(
            "defaults.no_audio_processing", val);
        bwp::wallpaper::WallpaperManager::getInstance().setNoAudioProcessing(
            val);
      }),
      nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(weGroup), noAudioProcRow);

  GtkWidget *noAutomuteRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(noAutomuteRow),
                                "No Auto-Mute");
  adw_action_row_set_subtitle(
      ADW_ACTION_ROW(noAutomuteRow),
      "Prevent wallpaper from auto-muting when other audio plays");
  adw_switch_row_set_active(ADW_SWITCH_ROW(noAutomuteRow),
                            conf.get<bool>("defaults.no_automute", false));
  g_signal_connect(
      noAutomuteRow, "notify::active",
      G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
        bool val = static_cast<bool>(adw_switch_row_get_active(r));
        bwp::config::ConfigManager::getInstance().set("defaults.no_automute",
                                                      val);
        bwp::wallpaper::WallpaperManager::getInstance().setNoAutomute(val);
      }),
      nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(weGroup), noAutomuteRow);

  GtkWidget *fullscreenRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(fullscreenRow),
                                "Pause on Fullscreen App");
  adw_action_row_set_subtitle(
      ADW_ACTION_ROW(fullscreenRow),
      "Pause wallpaper when a fullscreen application is running");
  adw_switch_row_set_active(
      ADW_SWITCH_ROW(fullscreenRow),
      conf.get<bool>("performance.pause_on_fullscreen", true));
  g_signal_connect(fullscreenRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "performance.pause_on_fullscreen",
                         static_cast<bool>(adw_switch_row_get_active(r)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(weGroup), fullscreenRow);

  return page;
}
GtkWidget *SettingsView::createTransitionsPage() {
  GtkWidget *page = adw_preferences_page_new();
  auto &conf = bwp::config::ConfigManager::getInstance();
  GtkWidget *transGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(transGroup),
                                  "Wallpaper Transitions");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(transGroup),
      "Configure how wallpapers transition when switching");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(transGroup));
  m_transitionEnabledSwitch = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_transitionEnabledSwitch),
                                "Enable Transitions");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(m_transitionEnabledSwitch),
                              "Smooth animation when changing wallpapers");
  adw_switch_row_set_active(ADW_SWITCH_ROW(m_transitionEnabledSwitch),
                            conf.get<bool>("transitions.enabled", true));
  g_signal_connect(m_transitionEnabledSwitch, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "transitions.enabled",
                         static_cast<bool>(adw_switch_row_get_active(row)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(transGroup),
                            m_transitionEnabledSwitch);
  m_transitionEffectDropdown = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_transitionEffectDropdown),
                                "Transition Effect");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(m_transitionEffectDropdown),
                              "Visual style of the transition");
  const char *effects[] = {
      "Fade",     "Slide", "Wipe",  "Expanding Circle", "Expanding Square",
      "Dissolve", "Zoom",  "Morph", "Angled Wipe",      "Pixelate",
      "Blinds",   nullptr};
  GtkStringList *effectModel = gtk_string_list_new(effects);
  adw_combo_row_set_model(ADW_COMBO_ROW(m_transitionEffectDropdown),
                          G_LIST_MODEL(effectModel));
  g_object_unref(effectModel);
  std::string currentEffect =
      conf.get<std::string>("transitions.default_effect", "Fade");
  int effectIdx = 0;
  for (int effectIndex = 0; effects[effectIndex] != nullptr; ++effectIndex) {
    if (currentEffect == effects[effectIndex]) {
      effectIdx = effectIndex;
      break;
    }
  }
  adw_combo_row_set_selected(ADW_COMBO_ROW(m_transitionEffectDropdown),
                             effectIdx);
  g_signal_connect(m_transitionEffectDropdown, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     guint i = adw_combo_row_get_selected(row);
                     std::string e;
                     switch (i) {
                     case 0:
                       e = "Fade";
                       break;
                     case 1:
                       e = "Slide";
                       break;
                     case 2:
                       e = "Wipe";
                       break;
                     case 3:
                       e = "Expanding Circle";
                       break;
                     case 4:
                       e = "Expanding Square";
                       break;
                     case 5:
                       e = "Dissolve";
                       break;
                     case 6:
                       e = "Zoom";
                       break;
                     case 7:
                       e = "Morph";
                       break;
                     case 8:
                       e = "Angled Wipe";
                       break;
                     case 9:
                       e = "Pixelate";
                       break;
                     case 10:
                       e = "Blinds";
                       break;
                     default:
                       return;
                     }
                     bwp::config::ConfigManager::getInstance().set(
                         "transitions.default_effect", e);
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(transGroup),
                            m_transitionEffectDropdown);
  m_transitionDurationSpin = adw_spin_row_new_with_range(100, 3000, 50);
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_transitionDurationSpin),
                                "Duration (ms)");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(m_transitionDurationSpin),
                              "How long the transition takes");
  adw_spin_row_set_value(ADW_SPIN_ROW(m_transitionDurationSpin),
                         conf.get<int>("transitions.duration_ms", 500));
  g_signal_connect(m_transitionDurationSpin, "notify::value",
                   G_CALLBACK(+[](AdwSpinRow *row, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "transitions.duration_ms",
                         static_cast<int>(adw_spin_row_get_value(row)));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(transGroup),
                            m_transitionDurationSpin);
  m_transitionEasingDropdown = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_transitionEasingDropdown),
                                "Easing Function");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(m_transitionEasingDropdown),
                              "How the animation accelerates/decelerates");
  const char *easings[] = {"linear",        "easeIn",       "easeOut",
                           "easeInOut",     "easeInQuad",   "easeOutQuad",
                           "easeInCubic",   "easeOutCubic", "easeInOutCubic",
                           "easeInSine",    "easeOutSine",  "easeInOutSine",
                           "easeInExpo",    "easeOutExpo",  "easeInOutExpo",
                           "easeOutBounce", nullptr};
  GtkStringList *easingModel = gtk_string_list_new(easings);
  adw_combo_row_set_model(ADW_COMBO_ROW(m_transitionEasingDropdown),
                          G_LIST_MODEL(easingModel));
  g_object_unref(easingModel);
  std::string currentEasing =
      conf.get<std::string>("transitions.easing", "easeInOut");
  int easingIdx = 3;
  for (int easingIndex = 0; easings[easingIndex] != nullptr; ++easingIndex) {
    if (currentEasing == easings[easingIndex]) {
      easingIdx = easingIndex;
      break;
    }
  }
  adw_combo_row_set_selected(ADW_COMBO_ROW(m_transitionEasingDropdown),
                             easingIdx);
  g_signal_connect(m_transitionEasingDropdown, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     guint i = adw_combo_row_get_selected(row);
                     std::string e;
                     switch (i) {
                     case 0:
                       e = "linear";
                       break;
                     case 1:
                       e = "easeIn";
                       break;
                     case 2:
                       e = "easeOut";
                       break;
                     case 3:
                       e = "easeInOut";
                       break;
                     case 4:
                       e = "easeInQuad";
                       break;
                     case 5:
                       e = "easeOutQuad";
                       break;
                     case 6:
                       e = "easeInCubic";
                       break;
                     case 7:
                       e = "easeOutCubic";
                       break;
                     case 8:
                       e = "easeInOutCubic";
                       break;
                     case 9:
                       e = "easeInSine";
                       break;
                     case 10:
                       e = "easeOutSine";
                       break;
                     case 11:
                       e = "easeInOutSine";
                       break;
                     case 12:
                       e = "easeInExpo";
                       break;
                     case 13:
                       e = "easeOutExpo";
                       break;
                     case 14:
                       e = "easeInOutExpo";
                       break;
                     case 15:
                       e = "easeOutBounce";
                       break;
                     default:
                       return;
                     }
                     bwp::config::ConfigManager::getInstance().set(
                         "transitions.easing", e);
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(transGroup),
                            m_transitionEasingDropdown);
  GtkWidget *previewRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(previewRow),
                                "Preview Transition");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(previewRow),
                              "Test your transition settings");
  GtkWidget *previewBtn = gtk_button_new_with_label("Preview");
  gtk_widget_add_css_class(previewBtn, "suggested-action");
  gtk_widget_set_valign(previewBtn, GTK_ALIGN_CENTER);
  g_signal_connect(previewBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     SettingsView *self = static_cast<SettingsView *>(data);
                     self->showTransitionPreviewDialog();
                   }),
                   this);
  adw_action_row_add_suffix(ADW_ACTION_ROW(previewRow), previewBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(transGroup), previewRow);
  GtkWidget *advGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(advGroup),
                                  "Advanced Options");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(advGroup));
  GtkWidget *infoRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(infoRow),
                                "Wallpaper Engine Transitions");
  adw_action_row_set_subtitle(
      ADW_ACTION_ROW(infoRow),
      "For linux-wallpaperengine wallpapers, the new wallpaper fades in over "
      "the old one using a custom overlay system.");
  GtkWidget *infoIcon =
      gtk_image_new_from_icon_name("dialog-information-symbolic");
  adw_action_row_add_prefix(ADW_ACTION_ROW(infoRow), infoIcon);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(advGroup), infoRow);
  return page;
}
void SettingsView::showTransitionPreviewDialog() {
  if (!m_transitionDialog) {
    GtkRoot *root = gtk_widget_get_root(m_content);
    GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
    m_transitionDialog = std::make_unique<TransitionDialog>(window);
    m_transitionDialog->setCallback(
        [this](const TransitionDialog::TransitionSettings &settings) {
          auto &conf = bwp::config::ConfigManager::getInstance();
          conf.set("transitions.default_effect", settings.effectName);
          conf.set("transitions.duration_ms", settings.durationMs);
          conf.set("transitions.easing", settings.easingName);
          const char *effects[] = {"Fade",
                                   "Slide",
                                   "Wipe",
                                   "Expanding Circle",
                                   "Expanding Square",
                                   "Dissolve",
                                   "Zoom",
                                   "Morph",
                                   "Angled Wipe",
                                   "Pixelate",
                                   "Blinds"};
          for (int effectIndex = 0; effectIndex < 11; ++effectIndex) {
            if (settings.effectName == effects[effectIndex]) {
              adw_combo_row_set_selected(
                  ADW_COMBO_ROW(m_transitionEffectDropdown), effectIndex);
              break;
            }
          }
          adw_spin_row_set_value(ADW_SPIN_ROW(m_transitionDurationSpin),
                                 settings.durationMs);
          const char *easings[] = {
              "linear",       "easeIn",       "easeOut",
              "easeInOut",    "easeInQuad",   "easeOutQuad",
              "easeInCubic",  "easeOutCubic", "easeInOutCubic",
              "easeInSine",   "easeOutSine",  "easeInOutSine",
              "easeInExpo",   "easeOutExpo",  "easeInOutExpo",
              "easeOutBounce"};
          for (int easingIndex = 0; easingIndex < 16; ++easingIndex) {
            if (settings.easingName == easings[easingIndex]) {
              adw_combo_row_set_selected(
                  ADW_COMBO_ROW(m_transitionEasingDropdown), easingIndex);
              break;
            }
          }
        });
  }
  auto &conf = bwp::config::ConfigManager::getInstance();
  TransitionDialog::TransitionSettings current;
  current.effectName =
      conf.get<std::string>("transitions.default_effect", "Fade");
  current.durationMs = conf.get<int>("transitions.duration_ms", 500);
  current.easingName = conf.get<std::string>("transitions.easing", "easeInOut");
  m_transitionDialog->show(current);
  m_transitionDialog->presentTo(m_content);
}
GtkWidget *SettingsView::createControlsPage() {
  GtkWidget *page = adw_preferences_page_new();
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group),
                                  "Keyboard Shortcuts");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(group));
  auto addShortcut = [&](const char *title, const char *shortcut) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    GtkWidget *label = gtk_label_new(shortcut);
    gtk_widget_add_css_class(label, "dim-label");
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), label);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), row);
  };
  addShortcut("Next Wallpaper", "Ctrl + Right");
  addShortcut("Previous Wallpaper", "Ctrl + Left");
  addShortcut("Pause/Resume", "Space");
  addShortcut("Hide Window", "Ctrl + W");
  return page;
}
GtkWidget *SettingsView::createAboutPage() {
  GtkWidget *page = adw_preferences_page_new();
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(group));
  GtkWidget *logo = gtk_image_new_from_icon_name("betterwallpaper");
  gtk_image_set_pixel_size(GTK_IMAGE(logo), 64);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_top(box, 24);
  gtk_widget_set_margin_bottom(box, 24);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(box), logo);
  GtkWidget *title = gtk_label_new("BetterWallpaper");
  gtk_widget_add_css_class(title, "title-2");
  gtk_box_append(GTK_BOX(box), title);
  std::string verStr = "Version " + std::string(BWP_VERSION) + " Beta release";
  GtkWidget *version = gtk_label_new(verStr.c_str());
  gtk_widget_add_css_class(version, "dim-label");
  gtk_box_append(GTK_BOX(box), version);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), box);
  GtkWidget *ghRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ghRow),
                                "GitHub Repository");
  GtkWidget *ghBtn = gtk_button_new_from_icon_name("external-link-symbolic");
  gtk_widget_set_valign(ghBtn, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class(ghBtn, "flat");
  g_signal_connect(
      ghBtn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer) {
        GtkUriLauncher *launcher =
            gtk_uri_launcher_new("https://github.com/Misiix9/BetterWallpaper");
        gtk_uri_launcher_launch(
            launcher, GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn))), NULL,
            NULL, NULL);
        g_object_unref(launcher);
      }),
      nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(ghRow), ghBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), ghRow);
  GtkWidget *aurRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(aurRow), "AUR Package");
  GtkWidget *aurBtn = gtk_button_new_from_icon_name("external-link-symbolic");
  gtk_widget_set_valign(aurBtn, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class(aurBtn, "flat");
  g_signal_connect(
      aurBtn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer) {
        GtkUriLauncher *launcher = gtk_uri_launcher_new(
            "https://aur.archlinux.org/packages/betterwallpaper-git");
        gtk_uri_launcher_launch(
            launcher, GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn))), NULL,
            NULL, NULL);
        g_object_unref(launcher);
      }),
      nullptr);
  adw_action_row_add_suffix(ADW_ACTION_ROW(aurRow), aurBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), aurRow);
  return page;
}
} // namespace bwp::gui
