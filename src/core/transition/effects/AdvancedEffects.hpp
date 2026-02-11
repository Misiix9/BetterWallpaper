#pragma once
#include "../TransitionEffect.hpp"
#include <cmath>
namespace bwp::transition {
class ExpandingCircleEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Expanding Circle"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
  void setOrigin(double xRatio, double yRatio) {
    m_originXRatio = xRatio;
    m_originYRatio = yRatio;
  }
private:
  double m_originXRatio = 0.5;  
  double m_originYRatio = 0.5;
};
class ExpandingSquareEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Expanding Square"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
  void setOrigin(double xRatio, double yRatio) {
    m_originXRatio = xRatio;
    m_originYRatio = yRatio;
  }
  void setCornerRadius(double radius) { m_cornerRadius = radius; }
private:
  double m_originXRatio = 0.5;
  double m_originYRatio = 0.5;
  double m_cornerRadius = 0.0;  
};
class DissolveEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Dissolve"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
  void setBlockSize(int size) { m_blockSize = std::max(2, size); }
private:
  int m_blockSize = 8;
};
class ZoomEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Zoom"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
  void setZoomFactor(double factor) { m_zoomFactor = factor; }
private:
  double m_zoomFactor = 1.5;  
};
class MorphEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Morph"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
};
class AngledWipeEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Angled Wipe"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
  void setAngle(double degrees) { m_angleDegrees = degrees; }
  void setSoftEdge(bool soft) { m_softEdge = soft; }
  void setEdgeWidth(double width) { m_edgeWidth = width; }
private:
  double m_angleDegrees = 0.0;  
  bool m_softEdge = false;
  double m_edgeWidth = 50.0;  
};
class PixelateEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Pixelate"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
  void setMaxBlockSize(int size) { m_maxBlockSize = std::max(4, size); }
private:
  int m_maxBlockSize = 64;
};
class BlindsEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Blinds"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
  void setBlindCount(int count) { m_blindCount = std::max(2, count); }
  void setVertical(bool vertical) { m_vertical = vertical; }
private:
  int m_blindCount = 10;
  bool m_vertical = false;  
};
}  
