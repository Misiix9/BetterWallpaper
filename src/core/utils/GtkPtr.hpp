#pragma once
#include <gtk/gtk.h>
#include <utility>
namespace bwp::utils {
template <typename T> class GtkPtr {
public:
  GtkPtr() noexcept : m_ptr(nullptr) {}
  explicit GtkPtr(T *ptr, bool addRef = false) noexcept : m_ptr(ptr) {
    if (m_ptr && addRef) {
      g_object_ref(m_ptr);
    }
  }
  static GtkPtr fromFloating(T *ptr) noexcept {
    if (ptr && g_object_is_floating(ptr)) {
      g_object_ref_sink(ptr);
    }
    return GtkPtr(ptr, false);
  }
  ~GtkPtr() { reset(); }
  GtkPtr(const GtkPtr &) = delete;
  GtkPtr &operator=(const GtkPtr &) = delete;
  GtkPtr(GtkPtr &&other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }
  GtkPtr &operator=(GtkPtr &&other) noexcept {
    if (this != &other) {
      reset();
      m_ptr = other.m_ptr;
      other.m_ptr = nullptr;
    }
    return *this;
  }
  [[nodiscard]] T *get() const noexcept { return m_ptr; }
  T &operator*() const noexcept { return *m_ptr; }
  T *operator->() const noexcept { return m_ptr; }
  explicit operator bool() const noexcept { return m_ptr != nullptr; }
  [[nodiscard]] T *release() noexcept {
    T *ptr = m_ptr;
    m_ptr = nullptr;
    return ptr;
  }
  void reset(T *ptr = nullptr) noexcept {
    if (m_ptr) {
      g_object_unref(m_ptr);
    }
    m_ptr = ptr;
  }
  void swap(GtkPtr &other) noexcept { std::swap(m_ptr, other.m_ptr); }
  template <typename U> [[nodiscard]] U *as() const noexcept {
    return reinterpret_cast<U *>(m_ptr);
  }
private:
  T *m_ptr;
};
template <typename T> [[nodiscard]] GtkPtr<T> makeGtkPtr(T *ptr) {
  return GtkPtr<T>::fromFloating(ptr);
}
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
class GErrorPtr {
public:
  GErrorPtr() noexcept : m_error(nullptr) {}
  GError **getPtr() noexcept {
    reset();  
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
}  
