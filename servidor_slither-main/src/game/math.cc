#include "game/math.h"
#include <algorithm>

// Original simple segment check (kept for legacy/unused paths)
bool Math::intersect_segments(float p0_x, float p0_y, float p1_x, float p1_y,
                                     float p2_x, float p2_y, float p3_x, float p3_y) {
  const float s1_x = p1_x - p0_x;
  const float s1_y = p1_y - p0_y;
  const float s2_x = p3_x - p2_x;
  const float s2_y = p3_y - p2_y;

  const float d = (-s2_x * s1_y + s1_x * s2_y);
  static const float epsilon = 0.0001f;
  if (d <= epsilon && d >= -epsilon) {
    return false;
  }

  const float s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y));
  if (s < 0 || s > d) {
    return false;
  }

  const float t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x));
  return !(t < 0 || t > d);
}

float Math::distance_squared(float v_x, float v_y, float w_x, float w_y, float p_x, float p_y) {
  const float l2 = distance_squared(v_x, v_y, w_x, w_y);
  if (l2 == 0.0) {
    return distance_squared(p_x, p_y, w_x, w_x);
  }
  float t = ((p_x - v_x) * (w_x - v_x) + (p_y - v_y) * (w_y - v_y)) / l2;
  t = fmaxf(0, fminf(1, t));
  return distance_squared(p_x, p_y, v_x + t * (w_x - v_x), v_y + t * (w_y - v_y));
}

float Math::distance_squared(float p0_x, float p0_y, float p1_x, float p1_y) {
  const float dx = p0_x - p1_x;
  const float dy = p0_y - p1_y;
  return dx * dx + dy * dy;
}

int32_t Math::distance_squared(uint16_t p0_x, uint16_t p0_y, uint16_t p1_x, uint16_t p1_y) {
  const int32_t dx = p0_x - p1_x;
  const int32_t dy = p0_y - p1_y;
  return dx * dx + dy * dy;
}

// NEW: Standard distance squared
float Math::dist_sq(float x1, float y1, float x2, float y2) {
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

// NEW: Accurate Segment vs Segment intersection (Prevents tunneling)
// Corresponds to the logic in Main.as for 'isx' and 'isy' calculation
bool Math::check_intersection(float ax, float ay, float bx, float by, 
                              float cx, float cy, float dx, float dy) {
    float aa2 = by - ay;
    float bb2 = ax - bx;
    float cc2 = aa2 * ax + bb2 * ay;

    float aa1 = cy - dy;
    float bb1 = dx - cx;
    float cc1 = aa1 * dx + bb1 * dy;

    float det = aa1 * bb2 - aa2 * bb1;

    if (fabs(det) < 0.0001f) return false;

    float isx = (bb2 * cc1 - bb1 * cc2) / det;
    float isy = (aa1 * cc2 - aa2 * cc1) / det;

    // Check bounds for segment 1
    if (isx < std::min(ax, bx) || isx > std::max(ax, bx) ||
        isy < std::min(ay, by) || isy > std::max(ay, by)) return false;

    // Check bounds for segment 2
    if (isx < std::min(cx, dx) || isx > std::max(cx, dx) ||
        isy < std::min(cy, dy) || isy > std::max(cy, dy)) return false;

    return true;
}