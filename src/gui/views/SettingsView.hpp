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
  GtkWidget *createGeneralPage();
  GtkWidget *createControlsPage();
  GtkWidget *createGraphicsPage();
  GtkWidget *createAudioPage();
  GtkWidget *createPlaybackPage();
  GtkWidget *createSourcesPage();
  GtkWidget *createTransitionsPage();
  GtkWidget *createAboutPage();
  void setupLibraryList(GtkWidget *group);
  void updateLibraryList(GtkWidget *group);
  void onAddSource(GtkWidget *group);
  void onRemoveSource(GtkWidget *group, const std::string &path);
  void showTransitionPreviewDialog();
  void takeSnapshot();
  void showUnsavedBar();
  void hideUnsavedBar();
  void onKeepChanges();
  void onDiscardChanges();
  void rebuildPages();
  GtkWidget *m_outerBox = nullptr;     
  GtkWidget *m_content = nullptr;      
  GtkWidget *m_splitView = nullptr;
  GtkWidget *m_stack = nullptr;
  GtkWidget *m_sidebarList = nullptr;
  GtkWidget *m_searchEntry = nullptr;
  GtkWidget *m_unsavedRevealer = nullptr;
  bool m_hasUnsavedChanges = false;
  nlohmann::json m_configSnapshot;
  struct SettingsPageInfo {
    std::string id;
    std::string title;
    std::vector<std::string> keywords;  
  };
  std::vector<SettingsPageInfo> m_pageInfos;
  void filterSettings(const std::string &query);
  GtkWidget *m_currentLibraryGroup = nullptr;
  GtkWidget *m_transitionEffectDropdown = nullptr;
  GtkWidget *m_transitionDurationSpin = nullptr;
  GtkWidget *m_transitionEasingDropdown = nullptr;
  GtkWidget *m_transitionEnabledSwitch = nullptr;
  std::unique_ptr<TransitionDialog> m_transitionDialog;
};
}  
