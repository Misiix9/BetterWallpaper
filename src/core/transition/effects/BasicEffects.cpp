#include "BasicEffects.hpp"
namespace bwp::transition {
void FadeEffect::render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
                        double progress, int width, int height,
                        const TransitionParams &params) {
  if (progress < 0.0)
    progress = 0.0;
  if (progress > 1.0)
    progress = 1.0;
  cairo_save(cr);
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
  cairo_save(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint_with_alpha(cr, progress);
  cairo_restore(cr);
}
void SlideEffect::render(cairo_t *cr, cairo_surface_t *from,
                         cairo_surface_t *to, double progress, int width,
                         int height, const TransitionParams &params) {
  double x_offset = 0;
  double y_offset = 0;
  switch (params.direction) {
  case Direction::Left:  
    x_offset = width * (1.0 - progress);
    break;
  case Direction::Right:  
    x_offset = -width * (1.0 - progress);
    break;
  case Direction::Up:  
    y_offset = height * (1.0 - progress);
    break;
  case Direction::Down:  
    y_offset = -height * (1.0 - progress);
    break;
  }
  // Use "from" as background so we never show black (user expects no black frame).
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);
  cairo_save(cr);
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
void WipeEffect::render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
                        double progress, int width, int height,
                        const TransitionParams &params) {
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);
  cairo_save(cr);
  if (params.direction == Direction::Right) {
    cairo_rectangle(cr, width * (1.0 - progress), 0, width * progress, height);
  } else if (params.direction == Direction::Left) {
    cairo_rectangle(cr, 0, 0, width * progress, height);
  } else if (params.direction == Direction::Down) {
    cairo_rectangle(cr, 0, height * (1.0 - progress), width, height * progress);
  } else {
    // Up: reveal from top
    cairo_rectangle(cr, 0, 0, width, height * progress);
  }
  cairo_clip(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
}
}  
