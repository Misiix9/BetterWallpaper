#pragma once
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <functional>
#include <string>
namespace bwp::transition {
class Easing {
public:
  using EasingFunc = std::function<double(double)>;
  static double linear(double progress) { return progress; }
  static double easeInQuad(double progress) { return progress * progress; }
  static double easeOutQuad(double progress) {
    return 1.0 - (1.0 - progress) * (1.0 - progress);
  }
  static double easeInOutQuad(double progress) {
    return progress < 0.5 ? 2.0 * progress * progress
                          : 1.0 - std::pow(-2.0 * progress + 2.0, 2) / 2.0;
  }
  static double easeInCubic(double progress) {
    return progress * progress * progress;
  }
  static double easeOutCubic(double progress) {
    return 1.0 - std::pow(1.0 - progress, 3);
  }
  static double easeInOutCubic(double progress) {
    return progress < 0.5 ? 4.0 * progress * progress * progress
                          : 1.0 - std::pow(-2.0 * progress + 2.0, 3) / 2.0;
  }
  static double easeInQuart(double progress) {
    return progress * progress * progress * progress;
  }
  static double easeOutQuart(double progress) {
    return 1.0 - std::pow(1.0 - progress, 4);
  }
  static double easeInOutQuart(double progress) {
    return progress < 0.5 ? 8.0 * progress * progress * progress * progress
                          : 1.0 - std::pow(-2.0 * progress + 2.0, 4) / 2.0;
  }
  static double easeInSine(double progress) {
    return 1.0 - std::cos((progress * M_PI) / 2.0);
  }
  static double easeOutSine(double progress) {
    return std::sin((progress * M_PI) / 2.0);
  }
  static double easeInOutSine(double progress) {
    return -(std::cos(M_PI * progress) - 1.0) / 2.0;
  }
  static double easeInExpo(double progress) {
    return progress == 0.0 ? 0.0 : std::pow(2.0, 10.0 * progress - 10.0);
  }
  static double easeOutExpo(double progress) {
    return progress == 1.0 ? 1.0 : 1.0 - std::pow(2.0, -10.0 * progress);
  }
  static double easeInOutExpo(double progress) {
    if (progress == 0.0)
      return 0.0;
    if (progress == 1.0)
      return 1.0;
    return progress < 0.5
               ? std::pow(2.0, 20.0 * progress - 10.0) / 2.0
               : (2.0 - std::pow(2.0, -20.0 * progress + 10.0)) / 2.0;
  }
  static double easeInCirc(double progress) {
    return 1.0 - std::sqrt(1.0 - progress * progress);
  }
  static double easeOutCirc(double progress) {
    return std::sqrt(1.0 - std::pow(progress - 1.0, 2));
  }
  static double easeInOutCirc(double progress) {
    return progress < 0.5
               ? (1.0 - std::sqrt(1.0 - std::pow(2.0 * progress, 2))) / 2.0
               : (std::sqrt(1.0 - std::pow(-2.0 * progress + 2.0, 2)) + 1.0) /
                     2.0;
  }
  static double easeInBack(double progress) {
    const double overshootAmount = 1.70158;
    const double coefficient = overshootAmount + 1.0;
    return coefficient * progress * progress * progress -
           overshootAmount * progress * progress;
  }
  static double easeOutBack(double progress) {
    const double overshootAmount = 1.70158;
    const double coefficient = overshootAmount + 1.0;
    return 1.0 + coefficient * std::pow(progress - 1.0, 3) +
           overshootAmount * std::pow(progress - 1.0, 2);
  }
  static double easeInOutBack(double progress) {
    const double overshootAmount = 1.70158;
    const double adjustedOvershoot = overshootAmount * 1.525;
    return progress < 0.5
               ? (std::pow(2.0 * progress, 2) *
                  ((adjustedOvershoot + 1.0) * 2.0 * progress -
                   adjustedOvershoot)) /
                     2.0
               : (std::pow(2.0 * progress - 2.0, 2) *
                      ((adjustedOvershoot + 1.0) * (progress * 2.0 - 2.0) +
                       adjustedOvershoot) +
                  2.0) /
                     2.0;
  }
  static double easeInElastic(double progress) {
    if (progress == 0.0)
      return 0.0;
    if (progress == 1.0)
      return 1.0;
    const double period = (2.0 * M_PI) / 3.0;
    return -std::pow(2.0, 10.0 * progress - 10.0) *
           std::sin((progress * 10.0 - 10.75) * period);
  }
  static double easeOutElastic(double progress) {
    if (progress == 0.0)
      return 0.0;
    if (progress == 1.0)
      return 1.0;
    const double period = (2.0 * M_PI) / 3.0;
    return std::pow(2.0, -10.0 * progress) *
               std::sin((progress * 10.0 - 0.75) * period) +
           1.0;
  }
  static double easeInOutElastic(double progress) {
    if (progress == 0.0)
      return 0.0;
    if (progress == 1.0)
      return 1.0;
    const double period = (2.0 * M_PI) / 4.5;
    return progress < 0.5
               ? -(std::pow(2.0, 20.0 * progress - 10.0) *
                   std::sin((20.0 * progress - 11.125) * period)) /
                     2.0
               : (std::pow(2.0, -20.0 * progress + 10.0) *
                  std::sin((20.0 * progress - 11.125) * period)) /
                         2.0 +
                     1.0;
  }
  static double easeOutBounce(double progress) {
    const double bounceCoeff = 7.5625;
    const double divisor = 2.75;
    if (progress < 1.0 / divisor) {
      return bounceCoeff * progress * progress;
    } else if (progress < 2.0 / divisor) {
      double adjustedProgress = progress - 1.5 / divisor;
      return bounceCoeff * adjustedProgress * adjustedProgress + 0.75;
    } else if (progress < 2.5 / divisor) {
      double adjustedProgress = progress - 2.25 / divisor;
      return bounceCoeff * adjustedProgress * adjustedProgress + 0.9375;
    } else {
      double adjustedProgress = progress - 2.625 / divisor;
      return bounceCoeff * adjustedProgress * adjustedProgress + 0.984375;
    }
  }
  static double easeInBounce(double progress) {
    return 1.0 - easeOutBounce(1.0 - progress);
  }
  static double easeInOutBounce(double progress) {
    return progress < 0.5
               ? (1.0 - easeOutBounce(1.0 - 2.0 * progress)) / 2.0
               : (1.0 + easeOutBounce(2.0 * progress - 1.0)) / 2.0;
  }
  static EasingFunc cubicBezier(double x1, double y1, double x2, double y2) {
    return [x1, y1, x2, y2](double progress) -> double {
      double bezierTime = progress;
      for (int iteration = 0; iteration < 8; ++iteration) {
        double currentX = sampleCubicBezier(x1, x2, bezierTime);
        double derivative = sampleCubicBezierDerivative(x1, x2, bezierTime);
        if (std::abs(derivative) < 1e-6)
          break;
        bezierTime -= (currentX - progress) / derivative;
        bezierTime = std::max(0.0, std::min(1.0, bezierTime));
      }
      return sampleCubicBezier(y1, y2, bezierTime);
    };
  }
  static EasingFunc getByName(const std::string &name) {
    if (name == "linear")
      return linear;
    if (name == "easeIn" || name == "ease-in")
      return easeInQuad;
    if (name == "easeOut" || name == "ease-out")
      return easeOutQuad;
    if (name == "easeInOut" || name == "ease-in-out")
      return easeInOutQuad;
    if (name == "easeInQuad")
      return easeInQuad;
    if (name == "easeOutQuad")
      return easeOutQuad;
    if (name == "easeInOutQuad")
      return easeInOutQuad;
    if (name == "easeInCubic")
      return easeInCubic;
    if (name == "easeOutCubic")
      return easeOutCubic;
    if (name == "easeInOutCubic")
      return easeInOutCubic;
    if (name == "easeInSine")
      return easeInSine;
    if (name == "easeOutSine")
      return easeOutSine;
    if (name == "easeInOutSine")
      return easeInOutSine;
    if (name == "easeInExpo")
      return easeInExpo;
    if (name == "easeOutExpo")
      return easeOutExpo;
    if (name == "easeInOutExpo")
      return easeInOutExpo;
    if (name == "easeInCirc")
      return easeInCirc;
    if (name == "easeOutCirc")
      return easeOutCirc;
    if (name == "easeInOutCirc")
      return easeInOutCirc;
    if (name == "easeInBack")
      return easeInBack;
    if (name == "easeOutBack")
      return easeOutBack;
    if (name == "easeInOutBack")
      return easeInOutBack;
    if (name == "easeInElastic")
      return easeInElastic;
    if (name == "easeOutElastic")
      return easeOutElastic;
    if (name == "easeInOutElastic")
      return easeInOutElastic;
    if (name == "easeInBounce")
      return easeInBounce;
    if (name == "easeOutBounce")
      return easeOutBounce;
    if (name == "easeInOutBounce")
      return easeInOutBounce;
    return easeInOutQuad;
  }
  static std::vector<std::string> getAvailableNames() {
    return {"linear",         "easeIn",          "easeOut",
            "easeInOut",      "easeInQuad",      "easeOutQuad",
            "easeInOutQuad",  "easeInCubic",     "easeOutCubic",
            "easeInOutCubic", "easeInSine",      "easeOutSine",
            "easeInOutSine",  "easeInExpo",     "easeOutExpo",
            "easeInOutExpo",  "easeInCirc",      "easeOutCirc",
            "easeInOutCirc",  "easeInBack",      "easeOutBack",
            "easeInOutBack",  "easeInElastic",   "easeOutElastic",
            "easeInOutElastic", "easeInBounce",  "easeOutBounce",
            "easeInOutBounce"};
  }
private:
  static double sampleCubicBezier(double p1, double p2, double t) {
    double oneMinusT = 1.0 - t;
    return 3.0 * oneMinusT * oneMinusT * t * p1 +
           3.0 * oneMinusT * t * t * p2 + t * t * t;
  }
  static double sampleCubicBezierDerivative(double p1, double p2, double t) {
    double oneMinusT = 1.0 - t;
    return 3.0 * oneMinusT * oneMinusT * p1 +
           6.0 * oneMinusT * t * (p2 - p1) + 3.0 * t * t * (1.0 - p2);
  }
};
}  
