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

  GtkWidget *m_entry;
  SearchCallback m_callback;
};

} // namespace bwp::gui
