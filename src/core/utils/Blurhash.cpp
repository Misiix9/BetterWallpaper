#include "Blurhash.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
namespace bwp::utils::blurhash {
static constexpr const char *BASE83_CHARS =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    "#$%*+,-.:;=?@[]^_{|}~";
static int base83Decode(char c) {
  const char *p = std::strchr(BASE83_CHARS, c);
  return p ? static_cast<int>(p - BASE83_CHARS) : -1;
}
static int decodeBase83(const std::string &str, int from, int to) {
  int value = 0;
  for (int i = from; i < to; ++i) {
    int digit = base83Decode(str[i]);
    if (digit < 0)
      return -1;
    value = value * 83 + digit;
  }
  return value;
}
static std::string encodeBase83(int value, int length) {
  std::string result;
  result.resize(length);
  for (int i = length - 1; i >= 0; --i) {
    result[i] = BASE83_CHARS[value % 83];
    value /= 83;
  }
  return result;
}
static float srgbToLinear(int value) {
  float v = static_cast<float>(value) / 255.0f;
  if (v <= 0.04045f)
    return v / 12.92f;
  return std::pow((v + 0.055f) / 1.055f, 2.4f);
}
static int linearToSrgb(float value) {
  float v = std::clamp(value, 0.0f, 1.0f);
  if (v <= 0.0031308f)
    return static_cast<int>(std::round(v * 12.92f * 255.0f));
  return static_cast<int>(
      std::round((1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f) * 255.0f));
}
static float decodeMaxAC(int quantizedMaximumValue, float punch) {
  return (static_cast<float>(quantizedMaximumValue + 1) / 166.0f) * punch;
}
static int encodeMaxAC(float maximumValue) {
  return std::clamp(static_cast<int>(std::floor(maximumValue * 166.0f - 0.5f)),
                    0, 82);
}
struct Color {
  float r, g, b;
};
static Color decodeDC(int value) {
  int r = value >> 16;
  int g = (value >> 8) & 255;
  int b = value & 255;
  return {srgbToLinear(r), srgbToLinear(g), srgbToLinear(b)};
}
static int encodeDC(const Color &c) {
  int r = linearToSrgb(c.r);
  int g = linearToSrgb(c.g);
  int b = linearToSrgb(c.b);
  return (r << 16) + (g << 8) + b;
}
static Color decodeAC(int value, float maximumValue) {
  int quantR = value / (19 * 19);
  int quantG = (value / 19) % 19;
  int quantB = value % 19;
  auto signPow = [](float val, float exp) -> float {
    return std::copysign(std::pow(std::abs(val), exp), val);
  };
  return {signPow((static_cast<float>(quantR) - 9.0f) / 9.0f, 2.0f) *
              maximumValue,
          signPow((static_cast<float>(quantG) - 9.0f) / 9.0f, 2.0f) *
              maximumValue,
          signPow((static_cast<float>(quantB) - 9.0f) / 9.0f, 2.0f) *
              maximumValue};
}
static int encodeAC(const Color &c, float maximumValue) {
  auto signPow = [](float val, float exp) -> float {
    return std::copysign(std::pow(std::abs(val), exp), val);
  };
  int quantR = static_cast<int>(std::clamp(
      std::floor(signPow(c.r / maximumValue, 0.5f) * 9.0f + 9.5f), 0.0f,
      18.0f));
  int quantG = static_cast<int>(std::clamp(
      std::floor(signPow(c.g / maximumValue, 0.5f) * 9.0f + 9.5f), 0.0f,
      18.0f));
  int quantB = static_cast<int>(std::clamp(
      std::floor(signPow(c.b / maximumValue, 0.5f) * 9.0f + 9.5f), 0.0f,
      18.0f));
  return quantR * 19 * 19 + quantG * 19 + quantB;
}
bool isValid(const std::string &hash) {
  if (hash.size() < 6)
    return false;
  int sizeFlag = decodeBase83(hash, 0, 1);
  if (sizeFlag < 0)
    return false;
  int numY = (sizeFlag / 9) + 1;
  int numX = (sizeFlag % 9) + 1;
  size_t expectedLength = 4 + 2 * static_cast<size_t>(numX * numY - 1);
  return hash.size() == expectedLength;
}
std::vector<uint8_t> decode(const std::string &hash, int width, int height,
                            float punch) {
  if (!isValid(hash) || width <= 0 || height <= 0)
    return {};
  int sizeFlag = decodeBase83(hash, 0, 1);
  int numY = (sizeFlag / 9) + 1;
  int numX = (sizeFlag % 9) + 1;
  int quantisedMaximumValue = decodeBase83(hash, 1, 2);
  float maximumValue = decodeMaxAC(quantisedMaximumValue, punch);
  int totalComponents = numX * numY;
  std::vector<Color> colors(totalComponents);
  int dcValue = decodeBase83(hash, 2, 6);
  colors[0] = decodeDC(dcValue);
  for (int i = 1; i < totalComponents; ++i) {
    int acValue = decodeBase83(hash, 4 + i * 2, 4 + i * 2 + 2);
    colors[i] = decodeAC(acValue, maximumValue);
  }
  std::vector<uint8_t> pixels(width * height * 4);
  const float piW = static_cast<float>(M_PI) / static_cast<float>(width);
  const float piH = static_cast<float>(M_PI) / static_cast<float>(height);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float r = 0.0f, g = 0.0f, b = 0.0f;
      for (int j = 0; j < numY; ++j) {
        for (int i = 0; i < numX; ++i) {
          float basis = std::cos(piW * static_cast<float>(i) *
                                 (static_cast<float>(x) + 0.5f)) *
                        std::cos(piH * static_cast<float>(j) *
                                 (static_cast<float>(y) + 0.5f));
          const Color &color = colors[j * numX + i];
          r += color.r * basis;
          g += color.g * basis;
          b += color.b * basis;
        }
      }
      int idx = (y * width + x) * 4;
      pixels[idx + 0] = static_cast<uint8_t>(linearToSrgb(r));
      pixels[idx + 1] = static_cast<uint8_t>(linearToSrgb(g));
      pixels[idx + 2] = static_cast<uint8_t>(linearToSrgb(b));
      pixels[idx + 3] = 255;  
    }
  }
  return pixels;
}
std::string encode(const uint8_t *pixels, int width, int height,
                   int componentsX, int componentsY) {
  if (!pixels || width <= 0 || height <= 0 || componentsX < 1 ||
      componentsX > 9 || componentsY < 1 || componentsY > 9)
    return {};
  std::vector<Color> factors(componentsX * componentsY);
  const float piW = static_cast<float>(M_PI) / static_cast<float>(width);
  const float piH = static_cast<float>(M_PI) / static_cast<float>(height);
  for (int j = 0; j < componentsY; ++j) {
    for (int i = 0; i < componentsX; ++i) {
      float r = 0.0f, g = 0.0f, b = 0.0f;
      float normalisation = (i == 0 && j == 0) ? 1.0f : 2.0f;
      for (int y = 0; y < height; ++y) {
        float cosY =
            std::cos(piH * static_cast<float>(j) *
                     (static_cast<float>(y) + 0.5f));
        for (int x = 0; x < width; ++x) {
          float basis =
              cosY * std::cos(piW * static_cast<float>(i) *
                              (static_cast<float>(x) + 0.5f));
          int idx = (y * width + x) * 3;  
          r += basis * srgbToLinear(pixels[idx + 0]);
          g += basis * srgbToLinear(pixels[idx + 1]);
          b += basis * srgbToLinear(pixels[idx + 2]);
        }
      }
      float scale =
          normalisation / static_cast<float>(width * height);
      factors[j * componentsX + i] = {r * scale, g * scale, b * scale};
    }
  }
  std::string hash;
  int sizeFlag = (componentsX - 1) + (componentsY - 1) * 9;
  hash += encodeBase83(sizeFlag, 1);
  float maximumValue = 0.0f;
  if (componentsX * componentsY > 1) {
    for (int i = 1; i < componentsX * componentsY; ++i) {
      maximumValue = std::max({maximumValue, std::abs(factors[i].r),
                               std::abs(factors[i].g),
                               std::abs(factors[i].b)});
    }
  }
  int quantisedMaximumValue = encodeMaxAC(maximumValue);
  hash += encodeBase83(quantisedMaximumValue, 1);
  float realMaximumValue =
      (static_cast<float>(quantisedMaximumValue + 1)) / 166.0f;
  hash += encodeBase83(encodeDC(factors[0]), 4);
  for (int i = 1; i < componentsX * componentsY; ++i) {
    hash += encodeBase83(encodeAC(factors[i], realMaximumValue), 2);
  }
  return hash;
}
}  
