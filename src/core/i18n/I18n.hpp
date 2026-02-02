#pragma once

#include <libintl.h>
#include <locale.h>
#include <string>

// Translation macro - wraps gettext for easy string marking
#define _(str) bwp::i18n::I18n::translate(str)

// Mark string for extraction but don't translate (for static arrays, etc.)
#define N_(str) str

namespace bwp::i18n {

class I18n {
public:
  // Initialize i18n. Call this early in main().
  static void init(const std::string& domain = "betterwallpaper", 
                   const std::string& localeDir = "/usr/share/locale");
  
  // Translate a string
  static const char* translate(const char* str);
  
  // Get current language code (e.g., "en", "de", "fr")
  static std::string getCurrentLanguage();
  
  // Set language (restarts translation domain)
  static void setLanguage(const std::string& langCode);

private:
  static std::string s_domain;
  static std::string s_localeDir;
};

} // namespace bwp::i18n
