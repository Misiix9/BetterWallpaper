#include "../widgets/TransitionDialog.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <vector>

namespace bwp::gui {

class SettingsView {
public:
  SettingsView();
  ~SettingsView();

  GtkWidget *getWidget() const { return m_content; }

private:
  void setupUi();

  // Build the categorized Pages
  GtkWidget *createGeneralPage();
  GtkWidget *createControlsPage();
  GtkWidget *createGraphicsPage();
  GtkWidget *createAudioPage();
  GtkWidget *createPlaybackPage();
  GtkWidget *createSourcesPage();
  GtkWidget *createTransitionsPage();
  GtkWidget *createAboutPage();

  // Helpers
  void setupLibraryList(GtkWidget *group);
  void updateLibraryList(GtkWidget *group);
  void onAddSource(GtkWidget *group);
  void onRemoveSource(GtkWidget *group, const std::string &path);
  void showTransitionPreviewDialog();

  GtkWidget *m_content;
  GtkWidget *m_splitView;
  GtkWidget *m_stack;
  GtkWidget *m_sidebarList;

  GtkWidget *m_currentLibraryGroup = nullptr;

  // Transition settings widgets (for updates)
  GtkWidget *m_transitionEffectDropdown = nullptr;
  GtkWidget *m_transitionDurationSpin = nullptr;
  GtkWidget *m_transitionEasingDropdown = nullptr;
  GtkWidget *m_transitionEnabledSwitch = nullptr;

  // Transition preview dialog
  std::unique_ptr<TransitionDialog> m_transitionDialog;
};

} // namespace bwp::gui
