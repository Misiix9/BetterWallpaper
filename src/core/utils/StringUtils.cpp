#include "StringUtils.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace bwp::utils {

std::string StringUtils::trim(const std::string &str) {
  auto start = str.begin();
  while (start != str.end() && std::isspace(*start)) {
    start++;
  }

  auto end = str.end();
  do {
    end--;
  } while (std::distance(start, end) > 0 && std::isspace(*end));

  return std::string(start, end + 1);
}

std::string StringUtils::toLower(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string StringUtils::toUpper(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

std::vector<std::string> StringUtils::split(const std::string &str,
                                            char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

std::string StringUtils::join(const std::vector<std::string> &parts,
                              const std::string &delimiter) {
  std::string result;
  for (size_t i = 0; i < parts.size(); ++i) {
    result += parts[i];
    if (i < parts.size() - 1) {
      result += delimiter;
    }
  }
  return result;
}

std::string StringUtils::urlEncode(const std::string &str) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (char c : str) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
      continue;
    }
    escaped << std::uppercase;
    escaped << '%' << std::setw(2) << int((unsigned char)c);
    escaped << std::nouppercase;
  }
  return escaped.str();
}

std::string StringUtils::urlDecode(const std::string &str) {
  std::string ret;
  char ch;
  int i, ii;
  for (i = 0; i < (int)str.length(); i++) {
    if (str[i] != '%') {
      if (str[i] == '+')
        ret += ' ';
      else
        ret += str[i];
    } else {
      std::istringstream i_s(str.substr(i + 1, 2));
      if (i_s >> std::hex >> ii) {
        ch = static_cast<char>(ii);
        ret += ch;
        i = i + 2;
      } else {
        // Invalid sequence
        ret += '%';
      }
    }
  }
  return ret;
}

bool StringUtils::startsWith(const std::string &str,
                             const std::string &prefix) {
  if (prefix.size() > str.size())
    return false;
  return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool StringUtils::endsWith(const std::string &str, const std::string &suffix) {
  if (suffix.size() > str.size())
    return false;
  return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string StringUtils::replaceAll(std::string str, const std::string &from,
                                    const std::string &to) {
  if (from.empty())
    return str;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  return str;
}

} // namespace bwp::utils
