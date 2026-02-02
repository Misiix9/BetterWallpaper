#include "I18n.hpp"
#include "../utils/Logger.hpp"

namespace bwp::i18n {

std::string I18n::s_domain = "betterwallpaper";
std::string I18n::s_localeDir = "/usr/share/locale";

void I18n::init(const std::string& domain, const std::string& localeDir) {
  s_domain = domain;
  s_localeDir = localeDir;
  
  // Set locale from environment
  setlocale(LC_ALL, "");
  
  // Set up gettext domain
  bindtextdomain(s_domain.c_str(), s_localeDir.c_str());
  bind_textdomain_codeset(s_domain.c_str(), "UTF-8");
  textdomain(s_domain.c_str());
  
  LOG_INFO("I18n: Initialized with domain '" + s_domain + "' and locale dir '" + s_localeDir + "'");
}

const char* I18n::translate(const char* str) {
  return gettext(str);
}

std::string I18n::getCurrentLanguage() {
  const char* lang = setlocale(LC_MESSAGES, nullptr);
  if (lang) {
    std::string langStr(lang);
    // Extract just the language code (e.g., "en_US.UTF-8" -> "en")
    size_t pos = langStr.find('_');
    if (pos != std::string::npos) {
      return langStr.substr(0, pos);
    }
    pos = langStr.find('.');
    if (pos != std::string::npos) {
      return langStr.substr(0, pos);
    }
    return langStr;
  }
  return "en";
}

void I18n::setLanguage(const std::string& langCode) {
  // Set environment variable for locale
  std::string localeStr = langCode + ".UTF-8";
  
#ifdef _WIN32
  _putenv_s("LANGUAGE", langCode.c_str());
  _putenv_s("LC_ALL", localeStr.c_str());
#else
  setenv("LANGUAGE", langCode.c_str(), 1);
  setenv("LC_ALL", localeStr.c_str(), 1);
#endif
  
  // Re-initialize
  setlocale(LC_ALL, "");
  bindtextdomain(s_domain.c_str(), s_localeDir.c_str());
  textdomain(s_domain.c_str());
  
  LOG_INFO("I18n: Language changed to '" + langCode + "'");
}

} // namespace bwp::i18n
