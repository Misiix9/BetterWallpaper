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
};
class TransitionEffect {
public:
  virtual ~TransitionEffect() = default;
  virtual std::string getName() const = 0;
  virtual void render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
                      double progress, int width, int height,
                      const TransitionParams &params) = 0;
};
}  
