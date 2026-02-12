#pragma once
#include <adwaita.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <vector>
namespace bwp::gui {
class Application : public AdwApplication {
public:
  static Application *create();
  static Application *getInstance();

public:
  ~Application();

protected:
  Application();

public:
  int run(int argc, char **argv);

private:
  static void onActivate(GApplication *app, gpointer user_data);
  static void onStartup(GApplication *app, gpointer user_data);
  static int onCommandLine(GApplication *app, GApplicationCommandLine *cmdline,
                           gpointer user_data);
  static std::string findStylesheetPath();
  static void loadStylesheet();
  static void setupCssHotReload();
  static void onCssFileChanged(GFileMonitor *monitor, GFile *file,
                               GFile *other_file, GFileMonitorEvent event_type,
                               gpointer user_data);
  static void reloadStylesheet();
  AdwApplication *m_app;
  static GtkCssProvider *s_cssProvider;
  static GFileMonitor *s_cssMonitor;
  static std::string s_currentCssPath;
  static void ensureBackgroundServices();
  static bool spawnProcess(const std::string &name,
                           const std::string &relativePath);
};
} // namespace bwp::gui
