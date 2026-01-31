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
  GtkWidget *createAboutPage();

  // Helpers
  void setupLibraryList(GtkWidget *group);
  void updateLibraryList(GtkWidget *group); // Modified to take group
  void onAddSource(GtkWidget *group);
  void onRemoveSource(GtkWidget *group, const std::string &path);

  GtkWidget *m_content;
  GtkWidget *m_splitView; // AdwOverlaySplitView or container
  GtkWidget *m_stack;     // AdwViewStack
  GtkWidget *m_sidebarList;

  // Track library rows for dynamic updates (might need mapping to group)
  // Or simply rebuild the group content?
  // Let's store the current library group pointer if needed.
  GtkWidget *m_currentLibraryGroup = nullptr;

  // Transition preview dialog
  std::unique_ptr<TransitionDialog> m_transitionDialog;
};

} // namespace bwp::gui
