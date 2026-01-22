#include "Application.hpp"
#include "../utils/Logger.hpp"
#include "MainWindow.hpp"
#include <iostream>

namespace bwp::gui {

static Application *s_instance = nullptr;

Application *Application::create() {
  if (!s_instance) {
    s_instance = new Application();
  }
  return s_instance;
}

Application *Application::getInstance() { return s_instance; }

Application::Application() {
  m_app = adw_application_new("com.github.betterwallpaper",
                              G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(m_app, "activate", G_CALLBACK(onActivate), this);
  g_signal_connect(m_app, "startup", G_CALLBACK(onStartup), this);
}

Application::~Application() { g_object_unref(m_app); }

int Application::run(int argc, char **argv) {
  return g_application_run(G_APPLICATION(m_app), argc, argv);
}

void Application::onActivate(GApplication *app, gpointer user_data) {
  auto *self = static_cast<Application *>(user_data);

  // Create main window
  // We maintain MainWindow instance via userdata or singleton or just new?
  // Usually we attach it to the application.
  // For simplicity, create and show.
  auto *window = new MainWindow(ADW_APPLICATION(app));
  window->show();
}

void Application::onStartup(GApplication *app, gpointer user_data) {
  // Initialize things
  bwp::utils::Logger::log(bwp::utils::LogLevel::INFO, "Application startup");

  // Load CSS
  GtkCssProvider *provider = gtk_css_provider_new();

  const char *cssPath = "data/ui/style.css";
  if (!std::filesystem::exists(cssPath)) {
    cssPath = "../data/ui/style.css"; // Try parent relative
  }

  gtk_css_provider_load_from_path(provider, cssPath);

  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_object_unref(provider);
}

} // namespace bwp::gui
