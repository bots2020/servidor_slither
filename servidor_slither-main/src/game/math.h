#ifndef SRC_GAME_MATH_H_
#define SRC_GAME_MATH_H_

#include <cmath>
#include <cstdint>
#include <algorithm> // Added for min/max

struct Point {
  float x;
  float y;
};

struct Circle : Point {
  float r;
};

struct Rect {
  Point p0, p1;
};

class Math {
  Math() = delete;

 public:
  static constexpr float f_pi = 3.14159265358979323846f;
  static constexpr float f_2pi = 2.0f * f_pi;

  inline static float normalize_angle(float ang) {
    return ang - f_2pi * floorf(ang / f_2pi);
  }

  static bool intersect_segments(float p0_x, float p0_y, float p1_x, float p1_y,
                                 float p2_x, float p2_y, float p3_x, float p3_y);

  static float distance_squared(float v_x, float v_y, float w_x, float w_y, float p_x, float p_y);
  
  // Standard distance squared between two points
  static float dist_sq(float x1, float y1, float x2, float y2);

  // Segment vs Segment intersection (AS3 Logic)
  static bool check_intersection(float ax, float ay, float bx, float by, 
                                 float cx, float cy, float dx, float dy);

  static float distance_squared(float p0_x, float p0_y, float p1_x, float p1_y);
  static int32_t distance_squared(uint16_t p0_x, uint16_t p0_y, uint16_t p1_x, uint16_t p1_y);

  inline static bool intersect_circle(float c_x, float c_y, float p_x, float p_y, float r) {
    return distance_squared(c_x, c_y, p_x, p_y) <= r * r;
  }

  inline static float fast_sqrt(float val) {
    union {
      int tmp;
      float val;
    } u;
    u.val = val;
    u.tmp -= 1 << 23;
    u.tmp >>= 1;
    u.tmp += 1 << 29;
    return u.val;
  }

  inline static float fast_inv_sqrt(float x) {
    union {
      int tmp;
      float val;
    } u;
    const float xhalf = 0.5f * x;
    u.val = x;
    u.tmp = 0x5f3759df - (u.tmp >> 1);
    return u.val * (1.5f - xhalf * u.val * u.val);
  }
};

#endif  // SRC_GAME_MATH_H_