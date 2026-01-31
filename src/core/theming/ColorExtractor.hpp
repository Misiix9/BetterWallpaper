#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace bwp::theming {

/**
 * Represents a color in RGB format with utility methods.
 */
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  // Constructor
  Color() = default;
  Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

  // Convert to hex string "#RRGGBB"
  std::string toHex() const;

  // Convert to RGB string "rgb(r, g, b)"
  std::string toRgb() const;

  // Luminance (0.0 - 1.0)
  double luminance() const;

  // Saturation (0.0 - 1.0)
  double saturation() const;

  // Hue (0 - 360)
  int hue() const;

  // Distance to another color (Euclidean in RGB space)
  double distanceTo(const Color &other) const;

  // Is this a light color?
  bool isLight() const { return luminance() > 0.5; }

  // Is this a dark color?
  bool isDark() const { return !isLight(); }
};

/**
 * A color palette extracted from an image.
 */
struct ColorPalette {
  Color primary;    // Most dominant color
  Color secondary;  // Second dominant
  Color accent;     // Vibrant accent color
  Color background; // Best for background (usually dark or light)
  Color foreground; // Text color (contrasts background)

  std::vector<Color> allColors; // Full palette (8, 16, or 32 colors)

  // Check if palette is valid (non-empty)
  bool isValid() const { return !allColors.empty(); }
};

/**
 * Extracts dominant colors from images using k-means clustering.
 */
class ColorExtractor {
public:
  static ColorExtractor &getInstance();

  // Extract palette from an image file
  ColorPalette extractFromImage(const std::string &imagePath,
                                int paletteSize = 16);

  // Extract palette from raw pixel data (RGBA)
  ColorPalette extractFromPixels(const uint8_t *pixels, int width, int height,
                                 int paletteSize = 16);

  // Sort colors by different criteria
  static void sortByLuminance(std::vector<Color> &colors);
  static void sortBySaturation(std::vector<Color> &colors);
  static void sortByHue(std::vector<Color> &colors);

private:
  ColorExtractor() = default;
  ~ColorExtractor() = default;

  ColorExtractor(const ColorExtractor &) = delete;
  ColorExtractor &operator=(const ColorExtractor &) = delete;

  // K-means clustering implementation
  std::vector<Color> kMeansClustering(const std::vector<Color> &pixels, int k,
                                      int maxIterations = 20);

  // Select special colors from palette
  Color selectPrimary(const std::vector<Color> &colors);
  Color selectSecondary(const std::vector<Color> &colors, const Color &primary);
  Color selectAccent(const std::vector<Color> &colors);
  Color selectBackground(const std::vector<Color> &colors);
  Color selectForeground(const Color &background);
};

} // namespace bwp::theming
