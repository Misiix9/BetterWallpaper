#include "TagManager.hpp"
#include <algorithm>
#include <cctype>
#include <set>

namespace bwp::wallpaper {

TagManager &TagManager::getInstance() {
  static TagManager instance;
  return instance;
}

TagManager::TagManager() { initializeDefaultTags(); }

TagManager::~TagManager() {}

std::vector<std::string> TagManager::getDefaultTags() const {
  return {"Anime",  "Nature",   "Sci-Fi",    "Abstract",
          "Gaming", "Space",    "Fantasy",   "Minimalist",
          "Dark",   "Colorful", "Cyberpunk", "Landscape",
          "Art",    "Music",    "Movies",    "City"};
}

void TagManager::initializeDefaultTags() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_knownTags.empty()) {
    m_knownTags = getDefaultTags();
  }
}

std::vector<std::string> TagManager::getAllTags() const {
  // Copy known tags first to avoid holding lock during WallpaperLibrary call
  std::vector<std::string> knownTagsCopy;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    knownTagsCopy = m_knownTags;
  }

  std::set<std::string> tags(knownTagsCopy.begin(), knownTagsCopy.end());

  // Now safe to call WallpaperLibrary without holding our mutex
  auto wallpapers = WallpaperLibrary::getInstance().getAllWallpapers();
  for (const auto &wp : wallpapers) {
    for (const auto &tag : wp.tags) {
      tags.insert(tag);
    }
  }
  return std::vector<std::string>(tags.begin(), tags.end());
}

void TagManager::addTag(const std::string &tag) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (std::find(m_knownTags.begin(), m_knownTags.end(), tag) ==
      m_knownTags.end()) {
    m_knownTags.push_back(tag);
  }
}

void TagManager::removeTag(const std::string &tag) {
  std::lock_guard<std::mutex> lock(m_mutex);

  // Remove from known tags
  auto it = std::remove(m_knownTags.begin(), m_knownTags.end(), tag);
  if (it != m_knownTags.end()) {
    m_knownTags.erase(it, m_knownTags.end());
  }

  // Remove from all wallpapers
  auto all = WallpaperLibrary::getInstance().getAllWallpapers();
  for (const auto &wp : all) {
    untagWallpaper(wp.id, tag);
  }
}

void TagManager::tagWallpaper(const std::string &wallpaperId,
                              const std::string &tag) {
  auto &lib = WallpaperLibrary::getInstance();
  auto wpOpt = lib.getWallpaper(wallpaperId);
  if (wpOpt) {
    auto wp = *wpOpt;
    // Check duplicate
    if (std::find(wp.tags.begin(), wp.tags.end(), tag) == wp.tags.end()) {
      wp.tags.push_back(tag);
      lib.updateWallpaper(wp);
    }
  }

  // Also add to known tags
  addTag(tag);
}

void TagManager::untagWallpaper(const std::string &wallpaperId,
                                const std::string &tag) {
  auto &lib = WallpaperLibrary::getInstance();
  auto wpOpt = lib.getWallpaper(wallpaperId);
  if (wpOpt) {
    auto wp = *wpOpt;
    auto it = std::remove(wp.tags.begin(), wp.tags.end(), tag);
    if (it != wp.tags.end()) {
      wp.tags.erase(it, wp.tags.end());
      lib.updateWallpaper(wp);
    }
  }
}

int TagManager::levenshteinDistance(const std::string &s1,
                                    const std::string &s2) {
  const size_t m = s1.size();
  const size_t n = s2.size();

  if (m == 0)
    return static_cast<int>(n);
  if (n == 0)
    return static_cast<int>(m);

  std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

  for (size_t i = 0; i <= m; ++i)
    dp[i][0] = static_cast<int>(i);
  for (size_t j = 0; j <= n; ++j)
    dp[0][j] = static_cast<int>(j);

  for (size_t i = 1; i <= m; ++i) {
    for (size_t j = 1; j <= n; ++j) {
      int cost = (std::tolower(s1[i - 1]) == std::tolower(s2[j - 1])) ? 0 : 1;
      dp[i][j] = std::min({
          dp[i - 1][j] + 1,       // deletion
          dp[i][j - 1] + 1,       // insertion
          dp[i - 1][j - 1] + cost // substitution
      });
    }
  }

  return dp[m][n];
}

std::vector<std::string>
TagManager::fuzzyMatchTags(const std::string &query) const {
  if (query.empty()) {
    return getAllTags();
  }

  std::vector<std::string> allTags = getAllTags();
  std::vector<std::pair<int, std::string>> scored;

  std::string lowerQuery = query;
  std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(),
                 ::tolower);

  for (const auto &tag : allTags) {
    std::string lowerTag = tag;
    std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(),
                   ::tolower);

    // Exact prefix match gets best score
    if (lowerTag.find(lowerQuery) == 0) {
      scored.push_back({0, tag});
    }
    // Contains match
    else if (lowerTag.find(lowerQuery) != std::string::npos) {
      scored.push_back({1, tag});
    }
    // Fuzzy match with threshold
    else {
      int distance = levenshteinDistance(lowerQuery, lowerTag);
      int threshold = std::max(2, static_cast<int>(query.length()) / 2);
      if (distance <= threshold) {
        scored.push_back({distance + 2, tag}); // Offset to rank after contains
      }
    }
  }

  // Sort by score
  std::sort(scored.begin(), scored.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });

  std::vector<std::string> result;
  for (const auto &pair : scored) {
    result.push_back(pair.second);
  }
  return result;
}

} // namespace bwp::wallpaper
