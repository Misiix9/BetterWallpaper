#pragma once

#include <functional>
#include <gtk/gtk.h>
#include <memory>

namespace bwp::gui {

/**
 * @brief Lazy-loading wrapper for views
 *
 * This template enables deferred creation of views until they are first
 * accessed. It helps reduce startup time by not creating views that the user
 * may not need.
 *
 * Usage:
 *   LazyView<SettingsView> m_settingsView;
 *
 *   // In sidebar callback:
 *   m_settingsView.ensureLoaded([this](auto& view) {
 *     adw_view_stack_add_named(ADW_VIEW_STACK(m_contentStack),
 *                              view.getWidget(), "settings");
 *   });
 *
 *   // Or to get the view directly (creates if needed):
 *   auto& settings = m_settingsView.get();
 */
template <typename T> class LazyView {
public:
  /**
   * @brief Callback type for when a view is first loaded
   */
  using LoadCallback = std::function<void(T &)>;

  LazyView() = default;

  /**
   * @brief Construct with a callback to run after first load
   */
  explicit LazyView(LoadCallback onLoad) : m_onLoad(std::move(onLoad)) {}

  /**
   * @brief Set callback to run when view is first created
   */
  void setOnLoad(LoadCallback callback) { m_onLoad = std::move(callback); }

  /**
   * @brief Check if the view has been created
   */
  [[nodiscard]] bool isLoaded() const { return m_view != nullptr; }

  /**
   * @brief Get the view, creating it if necessary
   */
  T &get() {
    ensureLoaded(nullptr);
    return *m_view;
  }

  /**
   * @brief Get the view, creating it if necessary (const version)
   */
  const T &get() const { return const_cast<LazyView *>(this)->get(); }

  /**
   * @brief Ensure the view is loaded, optionally running a callback
   * @param additionalCallback Optional callback to run (only on first load)
   * @return Reference to the view
   */
  T &ensureLoaded(LoadCallback additionalCallback = nullptr) {
    if (!m_view) {
      m_view = std::make_unique<T>();

      // Run stored callback
      if (m_onLoad) {
        m_onLoad(*m_view);
      }

      // Run additional callback
      if (additionalCallback) {
        additionalCallback(*m_view);
      }
    }
    return *m_view;
  }

  /**
   * @brief Get widget from the view (creates if needed)
   */
  GtkWidget *getWidget() { return get().getWidget(); }

  /**
   * @brief Arrow operator for convenient access
   */
  T *operator->() { return &get(); }

  /**
   * @brief Arrow operator (const version)
   */
  const T *operator->() const { return &get(); }

  /**
   * @brief Unload the view to free memory (will be recreated on next access)
   */
  void unload() { m_view.reset(); }

private:
  std::unique_ptr<T> m_view;
  LoadCallback m_onLoad;
};

/**
 * @brief View factory for creating views with consistent setup
 *
 * This provides a centralized place for view creation configuration.
 */
class ViewFactory {
public:
  /**
   * @brief Create a view with standard initialization
   */
  template <typename T, typename... Args>
  static std::unique_ptr<T> create(Args &&...args) {
    auto view = std::make_unique<T>(std::forward<Args>(args)...);
    // Could add common initialization here
    return view;
  }
};

} // namespace bwp::gui
