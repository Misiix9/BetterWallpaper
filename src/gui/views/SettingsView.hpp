#include "../widgets/TransitionDialog.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace bwp::gui {

class SettingsView {
public:
  SettingsView();
  ~SettingsView();

  GtkWidget *getWidget() const { return m_outerBox; }

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

  // Save/Discard workflow
  void takeSnapshot();
  void showUnsavedBar();
  void hideUnsavedBar();
  void onKeepChanges();
  void onDiscardChanges();
  void rebuildPages();

  GtkWidget *m_outerBox = nullptr;    // Top-level vertical box
  GtkWidget *m_content = nullptr;     // For internal use (split view)
  GtkWidget *m_splitView = nullptr;
  GtkWidget *m_stack = nullptr;
  GtkWidget *m_sidebarList = nullptr;
  GtkWidget *m_searchEntry = nullptr;
  GtkWidget *m_unsavedRevealer = nullptr;
  bool m_hasUnsavedChanges = false;
  nlohmann::json m_configSnapshot;

  // Search/filter support
  struct SettingsPageInfo {
    std::string id;
    std::string title;
    std::vector<std::string> keywords; // Searchable terms for this page
  };
  std::vector<SettingsPageInfo> m_pageInfos;
  void filterSettings(const std::string &query);

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
