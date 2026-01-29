#pragma once

#include <gtk/gtk.h>
#include <utility>

namespace bwp::utils {

/**
 * @brief RAII wrapper for GObject-derived types (GTK widgets, etc.)
 *
 * This template provides automatic reference counting management for
 * GTK/GLib GObject types. It takes ownership of the object and ensures
 * proper cleanup when the wrapper goes out of scope.
 *
 * Usage:
 *   GtkPtr<GtkWidget> button(gtk_button_new_with_label("Click me"));
 *   gtk_box_append(GTK_BOX(box), button.get());
 *   // button will be unreffed when GtkPtr goes out of scope
 *
 *   // For floating references (most widget constructors):
 *   GtkPtr<GtkWidget> label =
 * GtkPtr<GtkWidget>::fromFloating(gtk_label_new("Text"));
 */
template <typename T> class GtkPtr {
public:
  /**
   * @brief Default constructor - creates an empty (null) pointer
   */
  GtkPtr() noexcept : m_ptr(nullptr) {}

  /**
   * @brief Construct from a raw pointer, taking ownership
   * @param ptr Raw pointer to the GObject (will be unreffed on destruction)
   * @param addRef If true, add a reference (use for non-floating refs)
   *
   * For most GTK widget constructors, the returned object has a "floating"
   * reference which is consumed when the widget is added to a container.
   * If you're not adding to a container, use fromFloating() or sink the ref.
   */
  explicit GtkPtr(T *ptr, bool addRef = false) noexcept : m_ptr(ptr) {
    if (m_ptr && addRef) {
      g_object_ref(m_ptr);
    }
  }

  /**
   * @brief Create from a floating reference (sinks the floating ref)
   *
   * Use this for objects returned by gtk_*_new() functions which have
   * a floating reference that would otherwise be consumed by a container.
   */
  static GtkPtr fromFloating(T *ptr) noexcept {
    if (ptr && g_object_is_floating(ptr)) {
      g_object_ref_sink(ptr);
    }
    return GtkPtr(ptr, false);
  }

  /**
   * @brief Destructor - releases the reference
   */
  ~GtkPtr() { reset(); }

  // Non-copyable (to avoid double-unref issues)
  GtkPtr(const GtkPtr &) = delete;
  GtkPtr &operator=(const GtkPtr &) = delete;

  /**
   * @brief Move constructor
   */
  GtkPtr(GtkPtr &&other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }

  /**
   * @brief Move assignment operator
   */
  GtkPtr &operator=(GtkPtr &&other) noexcept {
    if (this != &other) {
      reset();
      m_ptr = other.m_ptr;
      other.m_ptr = nullptr;
    }
    return *this;
  }

  /**
   * @brief Get the raw pointer
   */
  [[nodiscard]] T *get() const noexcept { return m_ptr; }

  /**
   * @brief Dereference operator
   */
  T &operator*() const noexcept { return *m_ptr; }

  /**
   * @brief Arrow operator
   */
  T *operator->() const noexcept { return m_ptr; }

  /**
   * @brief Check if pointer is valid (non-null)
   */
  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  /**
   * @brief Release ownership of the pointer without unreffing
   * @return The raw pointer
   */
  [[nodiscard]] T *release() noexcept {
    T *ptr = m_ptr;
    m_ptr = nullptr;
    return ptr;
  }

  /**
   * @brief Reset the pointer, unreffing the current object
   * @param ptr New pointer to take ownership of (nullptr by default)
   */
  void reset(T *ptr = nullptr) noexcept {
    if (m_ptr) {
      g_object_unref(m_ptr);
    }
    m_ptr = ptr;
  }

  /**
   * @brief Swap with another GtkPtr
   */
  void swap(GtkPtr &other) noexcept { std::swap(m_ptr, other.m_ptr); }

  /**
   * @brief Cast to a different type (for GTK type hierarchies)
   *
   * Example: GtkPtr<GtkWidget> widget = ...;
   *          auto button = widget.as<GtkButton>();
   */
  template <typename U> [[nodiscard]] U *as() const noexcept {
    return reinterpret_cast<U *>(m_ptr);
  }

private:
  T *m_ptr;
};

/**
 * @brief Helper to create a GtkPtr from a floating reference
 */
template <typename T> [[nodiscard]] GtkPtr<T> makeGtkPtr(T *ptr) {
  return GtkPtr<T>::fromFloating(ptr);
}

/**
 * @brief RAII wrapper for GVariant
 */
class GVariantPtr {
public:
  GVariantPtr() noexcept : m_variant(nullptr) {}

  explicit GVariantPtr(GVariant *variant, bool addRef = false) noexcept
      : m_variant(variant) {
    if (m_variant && addRef) {
      g_variant_ref(m_variant);
    }
  }

  ~GVariantPtr() { reset(); }

  GVariantPtr(const GVariantPtr &) = delete;
  GVariantPtr &operator=(const GVariantPtr &) = delete;

  GVariantPtr(GVariantPtr &&other) noexcept : m_variant(other.m_variant) {
    other.m_variant = nullptr;
  }

  GVariantPtr &operator=(GVariantPtr &&other) noexcept {
    if (this != &other) {
      reset();
      m_variant = other.m_variant;
      other.m_variant = nullptr;
    }
    return *this;
  }

  [[nodiscard]] GVariant *get() const noexcept { return m_variant; }
  explicit operator bool() const noexcept { return m_variant != nullptr; }

  void reset(GVariant *variant = nullptr) noexcept {
    if (m_variant) {
      g_variant_unref(m_variant);
    }
    m_variant = variant;
  }

  [[nodiscard]] GVariant *release() noexcept {
    GVariant *v = m_variant;
    m_variant = nullptr;
    return v;
  }

private:
  GVariant *m_variant;
};

/**
 * @brief RAII wrapper for GError
 */
class GErrorPtr {
public:
  GErrorPtr() noexcept : m_error(nullptr) {}

  // Get pointer to internal error for use with GLib functions
  // that populate a GError** parameter
  // Usage: GErrorPtr err; someFunc(err.getPtr());
  GError **getPtr() noexcept {
    reset(); // Clear any existing error
    return &m_error;
  }

  ~GErrorPtr() { reset(); }

  GErrorPtr(const GErrorPtr &) = delete;
  GErrorPtr &operator=(const GErrorPtr &) = delete;

  GErrorPtr(GErrorPtr &&other) noexcept : m_error(other.m_error) {
    other.m_error = nullptr;
  }

  GErrorPtr &operator=(GErrorPtr &&other) noexcept {
    if (this != &other) {
      reset();
      m_error = other.m_error;
      other.m_error = nullptr;
    }
    return *this;
  }

  [[nodiscard]] GError *get() const noexcept { return m_error; }
  [[nodiscard]] const char *message() const noexcept {
    return m_error ? m_error->message : "";
  }
  [[nodiscard]] bool hasError() const noexcept { return m_error != nullptr; }
  explicit operator bool() const noexcept { return m_error != nullptr; }

  void reset() noexcept {
    if (m_error) {
      g_error_free(m_error);
      m_error = nullptr;
    }
  }

private:
  GError *m_error;
};

/**
 * @brief RAII wrapper for strings allocated by GLib (g_strdup, etc.)
 */
class GStringPtr {
public:
  GStringPtr() noexcept : m_str(nullptr) {}
  explicit GStringPtr(gchar *str) noexcept : m_str(str) {}

  ~GStringPtr() { reset(); }

  GStringPtr(const GStringPtr &) = delete;
  GStringPtr &operator=(const GStringPtr &) = delete;

  GStringPtr(GStringPtr &&other) noexcept : m_str(other.m_str) {
    other.m_str = nullptr;
  }

  GStringPtr &operator=(GStringPtr &&other) noexcept {
    if (this != &other) {
      reset();
      m_str = other.m_str;
      other.m_str = nullptr;
    }
    return *this;
  }

  [[nodiscard]] const gchar *get() const noexcept { return m_str; }
  [[nodiscard]] const gchar *c_str() const noexcept {
    return m_str ? m_str : "";
  }
  explicit operator bool() const noexcept { return m_str != nullptr; }

  void reset(gchar *str = nullptr) noexcept {
    if (m_str) {
      g_free(m_str);
    }
    m_str = str;
  }

  [[nodiscard]] gchar *release() noexcept {
    gchar *s = m_str;
    m_str = nullptr;
    return s;
  }

private:
  gchar *m_str;
};

} // namespace bwp::utils
