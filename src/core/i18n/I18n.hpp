#pragma once
#include <libintl.h>
#include <locale.h>
#include <string>
#define _(str) bwp::i18n::I18n::translate(str)
#define N_(str) str
namespace bwp::i18n {
class I18n {
public:
  static void init(const std::string& domain = "betterwallpaper", 
                   const std::string& localeDir = "/usr/share/locale");
  static const char* translate(const char* str);
  static std::string getCurrentLanguage();
  static void setLanguage(const std::string& langCode);
private:
  static std::string s_domain;
  static std::string s_localeDir;
};
}  
