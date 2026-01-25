#pragma once
#include "../TransitionEffect.hpp"

namespace bwp::transition {

class FadeEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Fade"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
};

class SlideEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Slide"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
};

class WipeEffect : public TransitionEffect {
public:
  std::string getName() const override { return "Wipe"; }
  void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
              double progress, int width, int height,
              const TransitionParams &params) override;
};

} // namespace bwp::transition
