#pragma once
#ifdef _WIN32
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
#else
#include <cairo.h>
#endif
#include <string>

namespace bwp::transition {

enum class Direction { Left, Right, Up, Down };

struct TransitionParams {
  Direction direction = Direction::Left;
  // Add other generic params if needed (e.g., smoothness, width)
};

class TransitionEffect {
public:
  virtual ~TransitionEffect() = default;

  virtual std::string getName() const = 0;

  /**
   * Render the transition frame.
   * @param cr Cairo context to draw on
   * @param from Surface of the old wallpaper
   * @param to Surface of the new wallpaper
   * @param progress Transition progress 0.0 to 1.0
   * @param width Width of the output
   * @param height Height of the output
   * @param params Additional parameters
   */
  virtual void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
                      double progress, int width, int height,
                      const TransitionParams &params) = 0;
};

} // namespace bwp::transition
