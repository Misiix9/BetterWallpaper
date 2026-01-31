#include "AdvancedEffects.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>

namespace bwp::transition {

// ============ Expanding Circle Effect ============

void ExpandingCircleEffect::render(cairo_t *cr, cairo_surface_t *from,
                                   cairo_surface_t *to, double progress,
                                   int width, int height,
                                   const TransitionParams &params) {
  // Draw the old wallpaper as background
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);

  // Calculate circle parameters
  double centerX = width * m_originXRatio;
  double centerY = height * m_originYRatio;

  // Maximum radius to cover the entire screen from origin
  double corner1 = std::pow(centerX, 2) + std::pow(centerY, 2);
  double corner2 = std::pow(width - centerX, 2) + std::pow(centerY, 2);
  double corner3 = std::pow(centerX, 2) + std::pow(height - centerY, 2);
  double corner4 = std::pow(width - centerX, 2) + std::pow(height - centerY, 2);
  double maxRadius = std::sqrt(
      std::max(std::max(corner1, corner2), std::max(corner3, corner4)));

  double currentRadius = maxRadius * progress;

  // Create circular clip and draw new wallpaper
  cairo_save(cr);
  cairo_new_path(cr);
  cairo_arc(cr, centerX, centerY, currentRadius, 0, 2 * M_PI);
  cairo_clip(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
}

// ============ Expanding Square Effect ============

void ExpandingSquareEffect::render(cairo_t *cr, cairo_surface_t *from,
                                   cairo_surface_t *to, double progress,
                                   int width, int height,
                                   const TransitionParams &params) {
  // Draw old wallpaper
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);

  double centerX = width * m_originXRatio;
  double centerY = height * m_originYRatio;

  // Max half-size to cover screen
  double maxHalfSize =
      std::max(std::max(centerX, width - centerX),
               std::max(centerY, height - centerY)) *
      1.5;

  double halfSize = maxHalfSize * progress;
  double cornerRad = m_cornerRadius * progress;

  // Create rounded rectangle clip
  cairo_save(cr);
  cairo_new_path(cr);

  double x = centerX - halfSize;
  double y = centerY - halfSize;
  double squareWidth = halfSize * 2;
  double squareHeight = halfSize * 2;

  if (cornerRad > 0) {
    // Rounded rectangle
    cairo_move_to(cr, x + cornerRad, y);
    cairo_line_to(cr, x + squareWidth - cornerRad, y);
    cairo_arc(cr, x + squareWidth - cornerRad, y + cornerRad, cornerRad,
              -M_PI / 2, 0);
    cairo_line_to(cr, x + squareWidth, y + squareHeight - cornerRad);
    cairo_arc(cr, x + squareWidth - cornerRad, y + squareHeight - cornerRad,
              cornerRad, 0, M_PI / 2);
    cairo_line_to(cr, x + cornerRad, y + squareHeight);
    cairo_arc(cr, x + cornerRad, y + squareHeight - cornerRad, cornerRad,
              M_PI / 2, M_PI);
    cairo_line_to(cr, x, y + cornerRad);
    cairo_arc(cr, x + cornerRad, y + cornerRad, cornerRad, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
  } else {
    cairo_rectangle(cr, x, y, squareWidth, squareHeight);
  }

  cairo_clip(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
}

// ============ Dissolve Effect ============

void DissolveEffect::render(cairo_t *cr, cairo_surface_t *from,
                            cairo_surface_t *to, double progress, int width,
                            int height, const TransitionParams &params) {
  // Draw the old wallpaper first
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);

  // Calculate block grid
  int blocksX = (width + m_blockSize - 1) / m_blockSize;
  int blocksY = (height + m_blockSize - 1) / m_blockSize;
  int totalBlocks = blocksX * blocksY;

  // Determine how many blocks to reveal based on progress
  int blocksToReveal = static_cast<int>(totalBlocks * progress);

  // Create a deterministic pseudo-random order based on block position
  // Using a simple hash for consistent results across frames
  cairo_save(cr);

  for (int by = 0; by < blocksY; ++by) {
    for (int bx = 0; bx < blocksX; ++bx) {
      // Simple hash for block reveal order
      int blockIndex = by * blocksX + bx;
      unsigned int hash = (blockIndex * 2654435761u) % totalBlocks;

      if (hash < static_cast<unsigned int>(blocksToReveal)) {
        // This block should show the new wallpaper
        int x = bx * m_blockSize;
        int y = by * m_blockSize;
        int blockWidth = std::min(m_blockSize, width - x);
        int blockHeight = std::min(m_blockSize, height - y);

        cairo_rectangle(cr, x, y, blockWidth, blockHeight);
      }
    }
  }

  cairo_clip(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
}

// ============ Zoom Effect ============

void ZoomEffect::render(cairo_t *cr, cairo_surface_t *from, cairo_surface_t *to,
                        double progress, int width, int height,
                        const TransitionParams &params) {
  double midPoint = 0.5;
  double centerX = width / 2.0;
  double centerY = height / 2.0;

  if (progress < midPoint) {
    // First half: zoom into old wallpaper while fading out
    double zoomProgress = progress / midPoint;
    double scale = 1.0 + (m_zoomFactor - 1.0) * zoomProgress;
    double alpha = 1.0 - zoomProgress;

    cairo_save(cr);
    cairo_translate(cr, centerX, centerY);
    cairo_scale(cr, scale, scale);
    cairo_translate(cr, -centerX, -centerY);
    cairo_set_source_surface(cr, from, 0, 0);
    cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
  } else {
    // Second half: zoom out from new wallpaper while fading in
    double zoomProgress = (progress - midPoint) / (1.0 - midPoint);
    double scale = m_zoomFactor - (m_zoomFactor - 1.0) * zoomProgress;
    double alpha = zoomProgress;

    cairo_save(cr);
    cairo_translate(cr, centerX, centerY);
    cairo_scale(cr, scale, scale);
    cairo_translate(cr, -centerX, -centerY);
    cairo_set_source_surface(cr, to, 0, 0);
    cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
  }
}

// ============ Morph Effect ============

void MorphEffect::render(cairo_t *cr, cairo_surface_t *from,
                         cairo_surface_t *to, double progress, int width,
                         int height, const TransitionParams &params) {
  // Simple crossfade with slight blur simulation via multiple paints
  // For a real morph, we'd need pixel-level operations

  // Draw source
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint_with_alpha(cr, 1.0 - progress);

  // Draw destination with additive blend
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint_with_alpha(cr, progress);
}

// ============ Angled Wipe Effect ============

void AngledWipeEffect::render(cairo_t *cr, cairo_surface_t *from,
                              cairo_surface_t *to, double progress, int width,
                              int height, const TransitionParams &params) {
  // Draw old wallpaper
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);

  double angleRad = m_angleDegrees * M_PI / 180.0;
  double cosAngle = std::cos(angleRad);
  double sinAngle = std::sin(angleRad);

  // Calculate the diagonal length for full coverage
  double diagonal = std::sqrt(width * width + height * height);

  // Wipe position along the angle direction
  double wipePos = progress * (diagonal + m_edgeWidth) - m_edgeWidth / 2;

  cairo_save(cr);

  if (m_softEdge && m_edgeWidth > 0) {
    // Create gradient mask for soft edge
    cairo_pattern_t *gradient = cairo_pattern_create_linear(
        width / 2.0 - cosAngle * diagonal, height / 2.0 - sinAngle * diagonal,
        width / 2.0 + cosAngle * diagonal, height / 2.0 + sinAngle * diagonal);

    double gradStart = (wipePos - m_edgeWidth / 2) / diagonal;
    double gradEnd = (wipePos + m_edgeWidth / 2) / diagonal;

    cairo_pattern_add_color_stop_rgba(gradient, std::max(0.0, gradStart), 0, 0,
                                      0, 0);
    cairo_pattern_add_color_stop_rgba(gradient, std::min(1.0, gradEnd), 0, 0, 0,
                                      1);

    cairo_set_source_surface(cr, to, 0, 0);
    cairo_mask(cr, gradient);
    cairo_pattern_destroy(gradient);
  } else {
    // Hard edge wipe using clip
    // Create a polygon that covers the revealed area
    cairo_new_path(cr);

    // Perpendicular direction for the wipe line
    double perpX = -sinAngle;
    double perpY = cosAngle;

    // Center of the wipe line
    double lineX = width / 2.0 + cosAngle * (wipePos - diagonal / 2);
    double lineY = height / 2.0 + sinAngle * (wipePos - diagonal / 2);

    // Four corners of the revealed area
    double extendLength = diagonal * 2;

    cairo_move_to(cr, lineX + perpX * extendLength,
                  lineY + perpY * extendLength);
    cairo_line_to(cr, lineX - perpX * extendLength,
                  lineY - perpY * extendLength);
    cairo_line_to(cr, lineX - perpX * extendLength + cosAngle * extendLength,
                  lineY - perpY * extendLength + sinAngle * extendLength);
    cairo_line_to(cr, lineX + perpX * extendLength + cosAngle * extendLength,
                  lineY + perpY * extendLength + sinAngle * extendLength);
    cairo_close_path(cr);
    cairo_clip(cr);

    cairo_set_source_surface(cr, to, 0, 0);
    cairo_paint(cr);
  }

  cairo_restore(cr);
}

// ============ Pixelate Effect ============

void PixelateEffect::render(cairo_t *cr, cairo_surface_t *from,
                            cairo_surface_t *to, double progress, int width,
                            int height, const TransitionParams &params) {
  double midPoint = 0.5;
  cairo_surface_t *source;
  double pixelProgress;

  if (progress < midPoint) {
    // First half: pixelate old wallpaper
    source = from;
    pixelProgress = progress / midPoint; // 0 to 1
  } else {
    // Second half: de-pixelate new wallpaper
    source = to;
    pixelProgress = 1.0 - (progress - midPoint) / (1.0 - midPoint); // 1 to 0
  }

  // Calculate block size based on progress
  int blockSize =
      1 + static_cast<int>((m_maxBlockSize - 1) * pixelProgress);

  if (blockSize <= 1) {
    // No pixelation needed
    cairo_set_source_surface(cr, source, 0, 0);
    cairo_paint(cr);
    return;
  }

  // Get source data for sampling
  cairo_surface_flush(source);
  unsigned char *sourceData = cairo_image_surface_get_data(source);
  int sourceStride = cairo_image_surface_get_stride(source);
  int sourceWidth = cairo_image_surface_get_width(source);
  int sourceHeight = cairo_image_surface_get_height(source);

  if (!sourceData || sourceWidth <= 0 || sourceHeight <= 0) {
    cairo_set_source_surface(cr, source, 0, 0);
    cairo_paint(cr);
    return;
  }

  // Draw pixelated version
  for (int by = 0; by < height; by += blockSize) {
    for (int bx = 0; bx < width; bx += blockSize) {
      // Sample center of block
      int sampleX = std::min(bx + blockSize / 2, sourceWidth - 1);
      int sampleY = std::min(by + blockSize / 2, sourceHeight - 1);

      unsigned char *pixel = sourceData + sampleY * sourceStride + sampleX * 4;
      double b = pixel[0] / 255.0;
      double g = pixel[1] / 255.0;
      double r = pixel[2] / 255.0;

      int blockWidth = std::min(blockSize, width - bx);
      int blockHeight = std::min(blockSize, height - by);

      cairo_set_source_rgb(cr, r, g, b);
      cairo_rectangle(cr, bx, by, blockWidth, blockHeight);
      cairo_fill(cr);
    }
  }
}

// ============ Blinds Effect ============

void BlindsEffect::render(cairo_t *cr, cairo_surface_t *from,
                          cairo_surface_t *to, double progress, int width,
                          int height, const TransitionParams &params) {
  // Draw old wallpaper as background
  cairo_set_source_surface(cr, from, 0, 0);
  cairo_paint(cr);

  // Calculate blind dimensions
  double blindSize =
      m_vertical ? (double)width / m_blindCount : (double)height / m_blindCount;

  // Reveal amount per blind
  double revealAmount = blindSize * progress;

  cairo_save(cr);
  cairo_new_path(cr);

  for (int blindIndex = 0; blindIndex < m_blindCount; ++blindIndex) {
    if (m_vertical) {
      // Vertical blinds (reveal from left of each blind)
      double x = blindIndex * blindSize;
      cairo_rectangle(cr, x, 0, revealAmount, height);
    } else {
      // Horizontal blinds (reveal from top of each blind)
      double y = blindIndex * blindSize;
      cairo_rectangle(cr, 0, y, width, revealAmount);
    }
  }

  cairo_clip(cr);
  cairo_set_source_surface(cr, to, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
}

} // namespace bwp::transition
