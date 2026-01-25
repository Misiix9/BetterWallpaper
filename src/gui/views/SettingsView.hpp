#include <adwaita.h>
#include <gtk/gtk.h>
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
  void setupLibraryPage();
  void setupGeneralPage();
  void setupAppearancePage();
  void setupPerformancePage();

  GtkWidget *m_content;
  GtkWidget *m_preferencesPage;
  GtkWidget *m_libraryGroup; // Store group to update list dynamically
  std::vector<GtkWidget *> m_libraryRows;

  void updateLibraryList();
  void onAddSource();
  void onRemoveSource(const std::string &path);
};

} // namespace bwp::gui
