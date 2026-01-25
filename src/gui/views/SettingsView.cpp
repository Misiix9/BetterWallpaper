#include "SettingsView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/LibraryScanner.hpp"
#include <algorithm>
#include <iostream>

namespace bwp::gui {

SettingsView::SettingsView() { setupUi(); }

SettingsView::~SettingsView() {}

void SettingsView::setupUi() {
  m_preferencesPage = adw_preferences_page_new();
  m_content = m_preferencesPage;

  setupLibraryPage();
  setupGeneralPage();
  setupAppearancePage();
  setupPerformancePage();
}

void SettingsView::setupLibraryPage() {
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group),
                                  "Library Sources");
  adw_preferences_group_set_description(
      ADW_PREFERENCES_GROUP(group), "Manage folders scanned for wallpapers");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(m_preferencesPage),
                           ADW_PREFERENCES_GROUP(group));
  m_libraryGroup = group;

  updateLibraryList();
}

void SettingsView::updateLibraryList() {
  // Clear tracked rows
  for (GtkWidget *row : m_libraryRows) {
    adw_preferences_group_remove(ADW_PREFERENCES_GROUP(m_libraryGroup), row);
  }
  m_libraryRows.clear();

  // Add "Add Source" button row
  GtkWidget *addRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(addRow), "Add New Source");

  GtkWidget *addBtn = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_set_valign(addBtn, GTK_ALIGN_CENTER);
  g_signal_connect_swapped(
      addBtn, "clicked",
      G_CALLBACK(+[](SettingsView *self) { self->onAddSource(); }), this);

  adw_action_row_add_suffix(ADW_ACTION_ROW(addRow), addBtn);
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_libraryGroup), addRow);
  m_libraryRows.push_back(addRow);

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

    g_signal_connect(delBtn, "clicked",
                     G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       SettingsView *self = static_cast<SettingsView *>(data);
                       std::string *p = static_cast<std::string *>(
                           g_object_get_data(G_OBJECT(btn), "path"));
                       if (p)
                         self->onRemoveSource(*p);
                     }),
                     this);

    adw_action_row_add_suffix(ADW_ACTION_ROW(row), delBtn);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_libraryGroup), row);
    m_libraryRows.push_back(row);
  }
}

void SettingsView::onAddSource() {
  auto dialog = gtk_file_chooser_native_new(
      "Select Wallpaper Folder", GTK_WINDOW(gtk_widget_get_root(m_content)),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "Select", "Cancel");

  g_signal_connect(
      dialog, "response",
      G_CALLBACK(+[](GtkNativeDialog *dialog, int response, gpointer data) {
        SettingsView *self = static_cast<SettingsView *>(data);
        if (response == GTK_RESPONSE_ACCEPT) {
          GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
          char *path = g_file_get_path(file);
          if (path) {
            auto &conf = bwp::config::ConfigManager::getInstance();
            auto paths = conf.get<std::vector<std::string>>("library.paths");

            // Add if not exists
            bool exists = false;
            for (const auto &p : paths)
              if (p == path)
                exists = true;

            if (!exists) {
              paths.push_back(path);
              conf.set("library.paths", paths);
              self->updateLibraryList();
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

void SettingsView::onRemoveSource(const std::string &path) {
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto paths = conf.get<std::vector<std::string>>("library.paths");

  auto it = std::remove(paths.begin(), paths.end(), path);
  if (it != paths.end()) {
    paths.erase(it, paths.end());
    conf.set("library.paths", paths);
    updateLibraryList();
    bwp::wallpaper::LibraryScanner::getInstance().scan(paths);
  }
}

void SettingsView::setupGeneralPage() {
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "General");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(m_preferencesPage),
                           ADW_PREFERENCES_GROUP(group));

  // Autostart
  GtkWidget *autostartRow = adw_switch_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(autostartRow),
                                "Start on Boot");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(autostartRow),
                              "Launch BetterWallpaper automatically");
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), autostartRow);
}

void SettingsView::setupAppearancePage() {
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Appearance");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(m_preferencesPage),
                           ADW_PREFERENCES_GROUP(group));

  GtkWidget *themeRow = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(themeRow), "Theme");
  adw_action_row_set_subtitle(ADW_ACTION_ROW(themeRow),
                              "Follows system preference");
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), themeRow);
}

void SettingsView::setupPerformancePage() {
  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Performance");
  adw_preferences_page_add(ADW_PREFERENCES_PAGE(m_preferencesPage),
                           ADW_PREFERENCES_GROUP(group));

  GtkWidget *fpsRow = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(fpsRow), "FPS Limit");

  const char *fps_opts[] = {"30", "60", "120", "Unlimited", NULL};
  GtkStringList *model = gtk_string_list_new(fps_opts);
  adw_combo_row_set_model(ADW_COMBO_ROW(fpsRow), G_LIST_MODEL(model));
  g_object_unref(model);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), fpsRow);
}

} // namespace bwp::gui
