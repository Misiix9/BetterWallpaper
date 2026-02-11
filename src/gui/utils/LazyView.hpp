#pragma once
#include <functional>
#include <gtk/gtk.h>
#include <memory>
namespace bwp::gui {
template <typename T> class LazyView {
public:
  using LoadCallback = std::function<void(T &)>;
  LazyView() = default;
  explicit LazyView(LoadCallback onLoad) : m_onLoad(std::move(onLoad)) {}
  void setOnLoad(LoadCallback callback) { m_onLoad = std::move(callback); }
  [[nodiscard]] bool isLoaded() const { return m_view != nullptr; }
  T &get() {
    ensureLoaded(nullptr);
    return *m_view;
  }
  const T &get() const { return const_cast<LazyView *>(this)->get(); }
  T &ensureLoaded(LoadCallback additionalCallback = nullptr) {
    if (!m_view) {
      m_view = std::make_unique<T>();
      if (m_onLoad) {
        m_onLoad(*m_view);
      }
      if (additionalCallback) {
        additionalCallback(*m_view);
      }
    }
    return *m_view;
  }
  GtkWidget *getWidget() { return get().getWidget(); }
  T *operator->() { return &get(); }
  const T *operator->() const { return &get(); }
  void unload() { m_view.reset(); }
private:
  std::unique_ptr<T> m_view;
  LoadCallback m_onLoad;
};
class ViewFactory {
public:
  template <typename T, typename... Args>
  static std::unique_ptr<T> create(Args &&...args) {
    auto view = std::make_unique<T>(std::forward<Args>(args)...);
    return view;
  }
};
}  
