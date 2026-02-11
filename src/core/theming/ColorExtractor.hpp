#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace bwp::theming {
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  Color() = default;
  Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
  std::string toHex() const;
  std::string toRgb() const;
  double luminance() const;
  double saturation() const;
  int hue() const;
  double distanceTo(const Color &other) const;
  bool isLight() const { return luminance() > 0.5; }
  bool isDark() const { return !isLight(); }
};
struct ColorPalette {
  Color primary;     
  Color secondary;   
  Color accent;      
  Color background;  
  Color foreground;  
  std::vector<Color> allColors;  
  bool isValid() const { return !allColors.empty(); }
};
class ColorExtractor {
public:
  static ColorExtractor &getInstance();
  ColorPalette extractFromImage(const std::string &imagePath,
                                int paletteSize = 16);
  ColorPalette extractFromPixels(const uint8_t *pixels, int width, int height,
                                 int paletteSize = 16);
  static void sortByLuminance(std::vector<Color> &colors);
  static void sortBySaturation(std::vector<Color> &colors);
  static void sortByHue(std::vector<Color> &colors);
private:
  ColorExtractor() = default;
  ~ColorExtractor() = default;
  ColorExtractor(const ColorExtractor &) = delete;
  ColorExtractor &operator=(const ColorExtractor &) = delete;
  std::vector<Color> kMeansClustering(const std::vector<Color> &pixels, int k,
                                      int maxIterations = 20);
  Color selectPrimary(const std::vector<Color> &colors);
  Color selectSecondary(const std::vector<Color> &colors, const Color &primary);
  Color selectAccent(const std::vector<Color> &colors);
  Color selectBackground(const std::vector<Color> &colors);
  Color selectForeground(const Color &background);
};
}  
