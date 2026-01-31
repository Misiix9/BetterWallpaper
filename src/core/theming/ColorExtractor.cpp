#include "ColorExtractor.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>

// For image loading we'll use gdk-pixbuf since we already have GTK
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace bwp::theming {

// Color implementation
std::string Color::toHex() const {
  char hex[8];
  snprintf(hex, sizeof(hex), "#%02x%02x%02x", r, g, b);
  return hex;
}

std::string Color::toRgb() const {
  std::ostringstream oss;
  oss << "rgb(" << static_cast<int>(r) << ", " << static_cast<int>(g) << ", "
      << static_cast<int>(b) << ")";
  return oss.str();
}

double Color::luminance() const {
  // Relative luminance formula
  double rn = r / 255.0;
  double gn = g / 255.0;
  double bn = b / 255.0;
  return 0.2126 * rn + 0.7152 * gn + 0.0722 * bn;
}

double Color::saturation() const {
  double rn = r / 255.0;
  double gn = g / 255.0;
  double bn = b / 255.0;

  double maxC = std::max({rn, gn, bn});
  double minC = std::min({rn, gn, bn});

  if (maxC == minC)
    return 0.0;

  double l = (maxC + minC) / 2.0;
  if (l <= 0.5) {
    return (maxC - minC) / (maxC + minC);
  }
  return (maxC - minC) / (2.0 - maxC - minC);
}

int Color::hue() const {
  double rn = r / 255.0;
  double gn = g / 255.0;
  double bn = b / 255.0;

  double maxC = std::max({rn, gn, bn});
  double minC = std::min({rn, gn, bn});
  double delta = maxC - minC;

  if (delta < 0.00001)
    return 0;

  double h = 0;
  if (maxC == rn) {
    h = 60.0 * fmod(((gn - bn) / delta), 6.0);
  } else if (maxC == gn) {
    h = 60.0 * (((bn - rn) / delta) + 2.0);
  } else {
    h = 60.0 * (((rn - gn) / delta) + 4.0);
  }

  if (h < 0)
    h += 360;
  return static_cast<int>(h);
}

double Color::distanceTo(const Color &other) const {
  int dr = static_cast<int>(r) - static_cast<int>(other.r);
  int dg = static_cast<int>(g) - static_cast<int>(other.g);
  int db = static_cast<int>(b) - static_cast<int>(other.b);
  return std::sqrt(dr * dr + dg * dg + db * db);
}

// ColorExtractor implementation
ColorExtractor &ColorExtractor::getInstance() {
  static ColorExtractor instance;
  return instance;
}

ColorPalette ColorExtractor::extractFromImage(const std::string &imagePath,
                                              int paletteSize) {
  ColorPalette palette;

  GError *error = nullptr;
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(imagePath.c_str(), &error);

  if (!pixbuf) {
    LOG_ERROR("Failed to load image for color extraction: " + imagePath);
    if (error) {
      LOG_ERROR(error->message);
      g_error_free(error);
    }
    return palette;
  }

  int width = gdk_pixbuf_get_width(pixbuf);
  int height = gdk_pixbuf_get_height(pixbuf);
  int channels = gdk_pixbuf_get_n_channels(pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

  // Sample pixels (don't process every pixel for large images)
  std::vector<Color> samples;
  int sampleStep = std::max(1, (width * height) / 10000); // Max ~10000 samples

  for (int y = 0; y < height; y += sampleStep) {
    for (int x = 0; x < width; x += sampleStep) {
      guchar *p = pixels + y * rowstride + x * channels;
      Color c(p[0], p[1], p[2]);

      // Skip very dark or very light colors (often background)
      if (c.luminance() > 0.05 && c.luminance() < 0.95) {
        samples.push_back(c);
      }
    }
  }

  g_object_unref(pixbuf);

  if (samples.empty()) {
    LOG_WARN("No suitable color samples found in image");
    return palette;
  }

  // Run k-means clustering
  std::vector<Color> colors = kMeansClustering(samples, paletteSize);

  // Sort by luminance for consistent ordering
  sortByLuminance(colors);

  palette.allColors = colors;
  palette.primary = selectPrimary(colors);
  palette.secondary = selectSecondary(colors, palette.primary);
  palette.accent = selectAccent(colors);
  palette.background = selectBackground(colors);
  palette.foreground = selectForeground(palette.background);

  LOG_INFO("Extracted " + std::to_string(colors.size()) + " colors from image");

  return palette;
}

ColorPalette ColorExtractor::extractFromPixels(const uint8_t *pixels, int width,
                                               int height, int paletteSize) {
  ColorPalette palette;
  std::vector<Color> samples;

  int sampleStep = std::max(1, (width * height) / 10000);

  for (int i = 0; i < width * height; i += sampleStep) {
    Color c(pixels[i * 4], pixels[i * 4 + 1], pixels[i * 4 + 2]);
    if (c.luminance() > 0.05 && c.luminance() < 0.95) {
      samples.push_back(c);
    }
  }

  if (samples.empty())
    return palette;

  std::vector<Color> colors = kMeansClustering(samples, paletteSize);
  sortByLuminance(colors);

  palette.allColors = colors;
  palette.primary = selectPrimary(colors);
  palette.secondary = selectSecondary(colors, palette.primary);
  palette.accent = selectAccent(colors);
  palette.background = selectBackground(colors);
  palette.foreground = selectForeground(palette.background);

  return palette;
}

std::vector<Color>
ColorExtractor::kMeansClustering(const std::vector<Color> &pixels, int k,
                                 int maxIterations) {
  if (pixels.empty() || k <= 0)
    return {};

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dist(0, pixels.size() - 1);

  // Initialize centroids randomly
  std::vector<Color> centroids;
  for (int i = 0; i < k; ++i) {
    centroids.push_back(pixels[dist(gen)]);
  }

  std::vector<int> assignments(pixels.size(), 0);

  for (int iter = 0; iter < maxIterations; ++iter) {
    // Assign pixels to nearest centroid
    for (size_t i = 0; i < pixels.size(); ++i) {
      double minDist = std::numeric_limits<double>::max();
      int nearest = 0;

      for (int j = 0; j < k; ++j) {
        double d = pixels[i].distanceTo(centroids[j]);
        if (d < minDist) {
          minDist = d;
          nearest = j;
        }
      }
      assignments[i] = nearest;
    }

    // Update centroids
    std::vector<int> counts(k, 0);
    std::vector<double> sumR(k, 0), sumG(k, 0), sumB(k, 0);

    for (size_t i = 0; i < pixels.size(); ++i) {
      int cluster = assignments[i];
      counts[cluster]++;
      sumR[cluster] += pixels[i].r;
      sumG[cluster] += pixels[i].g;
      sumB[cluster] += pixels[i].b;
    }

    bool converged = true;
    for (int j = 0; j < k; ++j) {
      if (counts[j] > 0) {
        Color newCentroid(static_cast<uint8_t>(sumR[j] / counts[j]),
                          static_cast<uint8_t>(sumG[j] / counts[j]),
                          static_cast<uint8_t>(sumB[j] / counts[j]));

        if (newCentroid.distanceTo(centroids[j]) > 1.0) {
          converged = false;
        }
        centroids[j] = newCentroid;
      }
    }

    if (converged)
      break;
  }

  return centroids;
}

void ColorExtractor::sortByLuminance(std::vector<Color> &colors) {
  std::sort(colors.begin(), colors.end(), [](const Color &a, const Color &b) {
    return a.luminance() < b.luminance();
  });
}

void ColorExtractor::sortBySaturation(std::vector<Color> &colors) {
  std::sort(colors.begin(), colors.end(), [](const Color &a, const Color &b) {
    return a.saturation() > b.saturation();
  });
}

void ColorExtractor::sortByHue(std::vector<Color> &colors) {
  std::sort(colors.begin(), colors.end(),
            [](const Color &a, const Color &b) { return a.hue() < b.hue(); });
}

Color ColorExtractor::selectPrimary(const std::vector<Color> &colors) {
  // Primary is the most saturated, mid-luminance color
  Color best;
  double bestScore = -1;

  for (const auto &c : colors) {
    double lum = c.luminance();
    double sat = c.saturation();

    // Prefer mid-luminance, high saturation
    double score = sat * (1.0 - std::abs(lum - 0.5));
    if (score > bestScore) {
      bestScore = score;
      best = c;
    }
  }

  return best;
}

Color ColorExtractor::selectSecondary(const std::vector<Color> &colors,
                                      const Color &primary) {
  // Secondary should be different from primary but still vibrant
  Color best;
  double bestScore = -1;

  for (const auto &c : colors) {
    double dist = c.distanceTo(primary);
    double sat = c.saturation();

    // Want good distance from primary and decent saturation
    double score = dist * sat;
    if (score > bestScore) {
      bestScore = score;
      best = c;
    }
  }

  return best;
}

Color ColorExtractor::selectAccent(const std::vector<Color> &colors) {
  // Accent is the most saturated color
  Color best;
  double maxSat = -1;

  for (const auto &c : colors) {
    if (c.saturation() > maxSat) {
      maxSat = c.saturation();
      best = c;
    }
  }

  return best;
}

Color ColorExtractor::selectBackground(const std::vector<Color> &colors) {
  // Background is the darkest or lightest color
  if (colors.empty())
    return Color(0, 0, 0);

  // Return darkest
  return colors.front();
}

Color ColorExtractor::selectForeground(const Color &background) {
  // Choose contrasting color for text
  if (background.isLight()) {
    return Color(30, 30, 30); // Dark text on light bg
  }
  return Color(240, 240, 240); // Light text on dark bg
}

} // namespace bwp::theming
