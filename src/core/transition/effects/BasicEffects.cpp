#include "BasicEffects.hpp"

namespace bwp::transition {

// FADE
void FadeEffect::render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
                        double progress, int width, int height,
                        const TransitionParams &params) {
  if (progress < 0.0)
    progress = 0.0;
  if (progress > 1.0)
    progress = 1.0;

  // Draw FROM
  cairo_save(cr);
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);

  // Draw TO with alpha
  cairo_save(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint_with_alpha(cr, progress);
  cairo_restore(cr);
}

// SLIDE
void SlideEffect::render(cairo_t *cr, cairo_surface_t *from,
                         cairo_surface_t *to, double progress, int width,
                         int height, const TransitionParams &params) {
  double x_offset = 0;
  double y_offset = 0;

  // "Slide over" or "Push"? Let's do Push for now.
  // Old moves OUT, New moves IN.

  switch (params.direction) {
  case Direction::Left: // New comes from Right
    x_offset = width * (1.0 - progress);
    break;
  case Direction::Right: // New comes from Left
    x_offset = -width * (1.0 - progress);
    break;
  case Direction::Up: // New comes from Bottom
    y_offset = height * (1.0 - progress);
    break;
  case Direction::Down: // New comes from Top
    y_offset = -height * (1.0 - progress);
    break;
  }

  // Clear bg?
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_paint(cr);

  // Draw FROM (Relative to new movement)
  cairo_save(cr);
  // If new comes from right (positive x_offset), old goes left (negative)
  double from_x = 0, from_y = 0;
  if (params.direction == Direction::Left)
    from_x = -width * progress;
  else if (params.direction == Direction::Right)
    from_x = width * progress;
  else if (params.direction == Direction::Up)
    from_y = -height * progress;
  else if (params.direction == Direction::Down)
    from_y = height * progress;

  cairo_set_source_surface(cr, from, from_x, from_y);
  cairo_paint(cr);
  cairo_restore(cr);

  // Draw TO
  cairo_save(cr);
  double to_x = 0, to_y = 0;
  if (params.direction == Direction::Left)
    to_x = width * (1.0 - progress);
  else if (params.direction == Direction::Right)
    to_x = -width * (1.0 - progress);
  else if (params.direction == Direction::Up)
    to_y = height * (1.0 - progress);
  else if (params.direction == Direction::Down)
    to_y = -height * (1.0 - progress);

  cairo_set_source_surface(cr, to, to_x, to_y);
  cairo_paint(cr);
  cairo_restore(cr);
}

// WIPE
void WipeEffect::render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
                        double progress, int width, int height,
                        const TransitionParams &params) {
  // Draw FROM
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);

  // Draw TO clipped
  cairo_save(cr);
  cairo_rectangle(cr, 0, 0, width * progress,
                  height); // Horizontal wipe left-to-right default
  if (params.direction == Direction::Right) {
    // Wipe Right (reveal from Left) -> rectangle 0 to width*progress
    // Actually Direction::Right usually means movement TO Right. So reveal from
    // left.
    cairo_rectangle(cr, 0, 0, width * progress, height);
  } else if (params.direction == Direction::Left) {
    // Wipe Left (reveal from Right)
    cairo_rectangle(cr, width * (1.0 - progress), 0, width, height);
  }
  // Simplification for now: always left to right for prototype

  cairo_clip(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
}

} // namespace bwp::transition
