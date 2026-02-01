#include "SettingsView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/system/AutostartManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/LibraryScanner.hpp"
#include <algorithm>
#include <iostream>

namespace bwp::gui {

SettingsView::SettingsView() { setupUi(); }

SettingsView::~SettingsView() {}

void SettingsView::setupUi() {
#if ADW_CHECK_VERSION(1, 4, 0)
  m_splitView = adw_overlay_split_view_new();
#else
  // Fallback for older adwaita
  m_splitView = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#endif
  m_content = m_splitView;

  // 1. Sidebar
  GtkWidget *sidebarBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(sidebarBox, 200, -1);
  gtk_widget_add_css_class(sidebarBox, "background");

  GtkWidget *header = gtk_label_new("Settings");
  gtk_widget_add_css_class(header, "title-2");
  gtk_widget_set_margin_top(header, 12);
  gtk_widget_set_margin_bottom(header, 12);
  gtk_box_append(GTK_BOX(sidebarBox), header);

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

  // 2. Content Stack
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
                     GtkWidget *pageWidget) {
  // Add to stack
#if ADW_CHECK_VERSION(1, 4, 0)
    adw_view_stack_add_named(ADW_VIEW_STACK(m_stack), pageWidget, id);
#else
    gtk_stack_add_named(GTK_STACK(m_stack), pageWidget, id);
#endif

    // Add sidebar row
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);

    gtk_box_append(GTK_BOX(box), gtk_image_new_from_icon_name(icon));
    gtk_box_append(GTK_BOX(box), gtk_label_new(title));
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);

    // Store ID
    g_object_set_data_full(G_OBJECT(row), "page_id", g_strdup(id), g_free);
    gtk_list_box_append(GTK_LIST_BOX(m_sidebarList), row);
  };

  // Create Pages
  addPage("general", "General", "emblem-system-symbolic", createGeneralPage());
  addPage("graphics", "Graphics", "video-display-symbolic",
          createGraphicsPage());
  addPage("audio", "Audio", "audio-volume-high-symbolic", createAudioPage());
  addPage("playback", "Playback", "media-playback-start-symbolic",
          createPlaybackPage());
  addPage("controls", "Controls", "input-keyboard-symbolic",
          createControlsPage());
  addPage("sources", "Sources", "folder-pictures-symbolic",
          createSourcesPage());
  addPage("about", "About", "help-about-symbolic", createAboutPage());

  // Navigation Logic
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

  // Select first
  GtkListBoxRow *first =
      gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_sidebarList), 0);
  if (first)
    gtk_list_box_select_row(GTK_LIST_BOX(m_sidebarList), first);
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
  // Clear existing content (except header/add button if we structure it that
  // way) AdwPreferencesGroup doesn't have "remove all". We have to track rows?
  // Or just rebuild the group every time?
  // The header struct has `m_libraryGroup` removed.
  // Let's assume we pass the group.

  // Actually, Adwaita doesn't easily let us clear.
  // A better way is to rebuild the page content or use a ListBox inside the
  // group? Use AdwPreferencesGroup with a ListBox? Let's follow the previous
  // pattern: remove specific rows if we track them. But we removed
  // `m_libraryRows` from header? No, I see `m_libraryRows` was implicitly
  // removed in header edit? Let's check header edit again. I replaced the
  // private section completely. `std::vector<GtkWidget *> m_libraryRows;` was
  // REMOVED.

  // So I cannot use `m_libraryRows`.
  // Alternative: Use a gtk_list_box inside the group's "child" (but prefs group
  // works with rows). Or: Just remove all children of the group? Iterating
  // children is possible.

  GtkWidget *child = gtk_widget_get_first_child(group);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    adw_preferences_group_remove(ADW_PREFERENCES_GROUP(group), child);
    child = next;
  }

  // Add "Add Source" button row
  GtkWidget *addRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(addRow), "Add New Source");
  GtkWidget *addBtn = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_set_valign(addBtn, GTK_ALIGN_CENTER);

  // We need to pass group to the callback
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

  // List existing sources
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

  // Store group on dialog to pass to response
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

  // Group: Application
  GtkWidget *appGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(appGroup),
                                  "Application");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(appGroup));

  auto &conf = bwp::config::ConfigManager::getInstance();

  // Autostart
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
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(appGroup), autostartRow);

  // Tray
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
                         adw_switch_row_get_active(row));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(appGroup), trayRow);

  // Window Mode
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

  // Group: Appearance (Theming)
  GtkWidget *themeGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(themeGroup),
                                  "Appearance & Theming");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(themeGroup));

  // Color Extraction
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
                         "theming.enabled", adw_switch_row_get_active(row));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(themeGroup), extractRow);

  // Backend Tool
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

  // Group: Backup
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

  return page;
}

GtkWidget *SettingsView::createGraphicsPage() {
  GtkWidget *page = adw_preferences_page_new();
  auto &conf = bwp::config::ConfigManager::getInstance();

  // Group: Performance
  GtkWidget *perfGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(perfGroup),
                                  "Performance");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(perfGroup));

  // FPS
  GtkWidget *fpsRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(fpsRow),
                                "Global FPS Limit");
  const char *fps_opts[] = {"30", "60", "120", "Unlimited", NULL};
  GtkStringList *model = gtk_string_list_new(fps_opts);
  adw_combo_row_set_model(ADW_COMBO_ROW(fpsRow), G_LIST_MODEL(model));
  g_object_unref(model);
  int currentFps = conf.get<int>("performance.fps_limit");
  int fpsIndex = 1;
  if (currentFps == 30)
    fpsIndex = 0;
  else if (currentFps == 120)
    fpsIndex = 2;
  else if (currentFps == 0)
    fpsIndex = 3;
  adw_combo_row_set_selected(ADW_COMBO_ROW(fpsRow), fpsIndex);
  g_signal_connect(fpsRow, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     int idx = adw_combo_row_get_selected(row);
                     int fps = 60;
                     switch (idx) {
                     case 0:
                       fps = 30;
                       break;
                     case 1:
                       fps = 60;
                       break;
                     case 2:
                       fps = 120;
                       break;
                     case 3:
                       fps = 0;
                       break;
                     }
                     bwp::config::ConfigManager::getInstance().set(
                         "performance.fps_limit", fps);
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(perfGroup), fpsRow);

  // Scaling
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
  adw_combo_row_set_selected(ADW_COMBO_ROW(scalingRow), s_idx);
  g_signal_connect(scalingRow, "notify::selected",
                   G_CALLBACK(+[](AdwComboRow *row, GParamSpec *, gpointer) {
                     // Simplification for brevity
                     int idx = adw_combo_row_get_selected(row);
                     const char *v = "fill";
                     if (idx == 1)
                       v = "fit";
                     else if (idx == 2)
                       v = "stretch";
                     bwp::config::ConfigManager::getInstance().set(
                         "defaults.scaling_mode", std::string(v));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(perfGroup), scalingRow);

  // Group: Resource Management
  GtkWidget *resourceGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(resourceGroup),
                                  "Resource Management");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(resourceGroup),
      "Limit memory usage to prevent system slowdown");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(resourceGroup));

  // RAM Limit
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

  // GPU Selection
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

  // Group: Content
  GtkWidget *contentGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(contentGroup),
                                  "Content");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(page),
                           ADW_PREFERENCES_GROUP(contentGroup));

  // NSFW Toggle
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
                         "content.show_nsfw", adw_switch_row_get_active(r));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(contentGroup), nsfwRow);

  // Group: Slideshow
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
                         "slideshow.shuffle", adw_switch_row_get_active(r));
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

  // Volume
  GtkWidget *volRow = adw_spin_row_new_with_range(0, 100, 1);
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(volRow),
                                "Global Volume Target");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(volRow),
                              "Default volume for new wallpapers");
  adw_spin_row_set_value(ADW_SPIN_ROW(volRow),
                         conf.get<int>("defaults.audio_volume"));
  g_signal_connect(volRow, "notify::value",
                   G_CALLBACK(+[](AdwSpinRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "defaults.audio_volume",
                         (int)adw_spin_row_get_value(r));
                   }),
                   nullptr);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), volRow);

  // Audio Enabled (Global Mute)
  GtkWidget *muteRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(muteRow), "Audio Enabled");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(muteRow),
                              "If disabled, all wallpapers will be muted");
  adw_switch_row_set_active(ADW_SWITCH_ROW(muteRow),
                            conf.get<bool>("defaults.audio_enabled"));
  g_signal_connect(muteRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "defaults.audio_enabled",
                         adw_switch_row_get_active(r));
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
                            conf.get<bool>("playback.pause_on_battery"));

  g_signal_connect(batRow, "notify::active",
                   G_CALLBACK(+[](AdwSwitchRow *r, GParamSpec *, gpointer) {
                     bwp::config::ConfigManager::getInstance().set(
                         "playback.pause_on_battery",
                         adw_switch_row_get_active(r));
                   }),
                   nullptr);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), batRow);
  return page;
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

  GtkWidget *version = gtk_label_new("Version 0.2.0");
  gtk_widget_add_css_class(version, "dim-label");
  gtk_box_append(GTK_BOX(box), version);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), box);

  GtkWidget *linkRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(linkRow),
                                "GitHub Repository");
  GtkWidget *linkBtn = gtk_button_new_from_icon_name("external-link-symbolic");
  gtk_widget_set_valign(linkBtn, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix(ADW_ACTION_ROW(linkRow), linkBtn);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), linkRow);
  return page;
}

} // namespace bwp::gui
