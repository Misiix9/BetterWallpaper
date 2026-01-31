#pragma once
#include <adwaita.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <gio/gio.h>
#include <gtk/gtk.h> // Ensure GTK is included
#include <memory>
#include <string>
#include <vector>

namespace bwp::gui {

class Application : public AdwApplication {
public:
  static Application *create();
  static Application *getInstance();

protected:
  Application();
  ~Application();

  // GApplication virtuals usually handled via signals in C++, but if wrapping:
  // With AdwApplication, usually we rely on signals or subclassing GObject.
  // In C++ with gtkmm, we inherit Gtk::Application.
  // Here we are using C API with C++ class wrapper/managers.
  // Wait, the plan says "Inherit from Gtk::Application".
  // This implies using gtkmm.
  // But my CMake uses `gtk4` (C library via PkgConfig).
  // If I want to use C++ classes inheriting from GTK classes, I MUST use
  // `gtkmm`. My `CMakeLists.txt` does NOT link `gtkmm`. Using `gtk4` (C) means
  // I cannot inherit C++ style from `GtkApplication`. I entered a dilemma: The
  // plan implies C++ architecture, but setup is C-style GTK. I should probably
  // switch to `gtkmm` dependency if I want proper C++ inheritance. Or I wrap
  // the C objects. Wrapper is cleaner if gtkmm is not available. User's
  // plan 1.3 says "GTK4 + libadwaita". Usually implies C or bindings. I'll
  // stick to C API wrapped in C++ classes (Composition, not Inheritance).
  // `Application` class will manage the `AdwApplication*` instance.

public:
  int run(int argc, char **argv);

private:
  static void onActivate(GApplication *app, gpointer user_data);
  static void onStartup(GApplication *app, gpointer user_data);

  // CSS loading and hot-reload
  static std::string findStylesheetPath();
  static void loadStylesheet();
  static void setupCssHotReload();
  static void onCssFileChanged(GFileMonitor *monitor, GFile *file,
                               GFile *other_file, GFileMonitorEvent event_type,
                               gpointer user_data);
  static void reloadStylesheet();

  AdwApplication *m_app;

  // CSS hot-reload state
  static GtkCssProvider *s_cssProvider;
  static GFileMonitor *s_cssMonitor;
  static std::string s_currentCssPath;

  // Background service management
  static void ensureBackgroundServices();
  static bool spawnProcess(const std::string &name,
                           const std::string &relativePath);
};

} // namespace bwp::gui
