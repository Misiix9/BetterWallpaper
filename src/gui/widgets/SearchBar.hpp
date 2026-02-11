#pragma once
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <string>
namespace bwp::gui {
class SearchBar {
public:
  SearchBar();
  ~SearchBar();
  GtkWidget *getWidget() const { return m_entry; }
  std::string getText() const;
  void setText(const std::string &text);
  using SearchCallback = std::function<void(const std::string &)>;
  void setCallback(SearchCallback callback);
private:
  static void onChanged(GtkSearchEntry *entry, gpointer user_data);
  static gboolean onTimeout(gpointer user_data);
  static void onEntryActivate(GtkSearchEntry *entry, gpointer user_data);
  void addToHistory(const std::string &query);
  void updateHistoryMenu();
  GtkWidget *m_entry;
  GtkWidget *m_popover;
  GtkWidget *m_historyBox;
  SearchCallback m_callback;
  guint m_timeoutId = 0;
};
}  
