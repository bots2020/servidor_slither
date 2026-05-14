/*
==================================================
FILE: snake.h
RELATIVE PATH: game/snake.h
==================================================
*/
#ifndef SRC_GAME_SNAKE_H_
#define SRC_GAME_SNAKE_H_

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional> 

#include "game/config.h"
#include "game/sector.h"

struct FoodEatenData {
  uint16_t x;
  uint16_t y;
  uint8_t size;
  uint8_t color;
};


// ... Enums ...
enum snake_changes_t : uint8_t {
  change_pos = 1,
  change_angle = 1 << 1,
  change_wangle = 1 << 2,
  change_speed = 1 << 3,
  change_fullness = 1 << 4,
  change_dying = 1 << 5,
  change_dead = 1 << 6
};

struct Body {
  float x; float y;
  inline void From(const Body &p) { x = p.x; y = p.y; }
  inline void Offset(float dx, float dy) { x += dx; y += dy; }
  inline float DistanceSquared(float dx, float dy) const {
    const float a = x - dx; const float b = y - dy; return a * a + b * b;
  }
};

typedef std::vector<Body> BodySeq;
typedef std::vector<Body>::const_iterator BodySeqCIter;

class Snake : public std::enable_shared_from_this<Snake> {
 public:
  typedef std::shared_ptr<Snake> Ptr;

  snake_id_t id;
  uint8_t skin;
  uint8_t update;
  bool acceleration;
  bool bot;

  // Flag: cobra já entrou no battledome pelo menos uma vez.
  // Usada para “entrar e tentar sair => morte”.
  bool in_battledome = false; // Estado persistente durante a vida.

  std::string name;
  std::string custom_skin_data;
  uint16_t speed;
  float angle;
  float wangle;
  uint16_t fullness;
  uint16_t target_score = 0; // Target score to grow to (spawn animation)

  SnakeBoundBox sbb;
  ViewPort vp;
  BodySeq parts;
  std::vector<FoodEatenData> eaten;
  FoodSeq spawn;
  size_t clientPartsIndex;

  // Physics Cache
  float sc13 = 1.0f; 
  float lsz = 29.0f; 

  // Bot State
  float bot_target_x = 0;
  float bot_target_y = 0;

  bool Tick(long dt, SectorSeq *ss, const WorldConfig &config);
  void TickAI(long frames, SectorSeq *ss);
  void UpdateBoxCenter();
  void UpdateBoxRadius();
  void UpdateSnakeConsts();
  void InitBoxNewSectors(SectorSeq *ss);
  void UpdateEatenFood(SectorSeq *ss);

  bool Intersect(BoundBoxPos foe) const;

  void IncreaseSnake(uint16_t volume);
  void DecreaseSnake(uint16_t volume, uint8_t drop_size);
  void SpawnFood(Food f);

  void on_dead_food_spawn(SectorSeq *ss, std::function<float()> next_randomf);
  void on_food_eaten(Food f);

  float get_snake_scale() const;
  float get_snake_body_part_radius() const;
  uint16_t get_snake_score() const;

  inline const Body &get_head() const { return parts[0]; }
  inline float get_head_x() const { return parts[0].x; }
  inline float get_head_y() const { return parts[0].y; }
  inline float get_head_dx() const { return parts[0].x - parts[1].x; }
  inline float get_head_dy() const { return parts[0].y - parts[1].y; }

  std::shared_ptr<Snake> get_ptr();
  BoundBox get_new_box() const;

  // Constants
  static constexpr float spangdv = 4.8f;
  static constexpr float nsp1 = 5.39f; 
  static constexpr float nsp2 = 0.4f;
  static constexpr float nsp3 = 14.0f; 
  static const uint16_t base_move_speed = 172; 
  static const uint16_t boost_speed = 448;     
  static const uint16_t speed_acceleration = 1000;
  static constexpr float snake_angular_speed = 4.125f; 
  static constexpr float prey_angular_speed = 3.625f;    
  static constexpr float snake_tail_k = 0.43f; 
  
  static const int parts_skip_count = 3;
  static const int parts_start_move_count = 4;
  static constexpr float tail_step_distance = 24.0f;
  // Ângulo por “step” de rotação (equilíbrio com a velocidade e boost).
  static constexpr float rot_step_angle =
      1.0f * WorldConfig::move_step_distance / boost_speed * snake_angular_speed;
  // Intervalo em ms para aplicar um step de rotação.
  static const long rot_step_interval =
      static_cast<long>(1000.0f * rot_step_angle / snake_angular_speed);
  static const long ai_step_interval = 250; 

 private:
  long mov_ticks = 0;
  long rot_ticks = 0;
  long ai_ticks = 0;

  void BotFindFood(SectorSeq *ss);
  bool BotCheckCollision(SectorSeq *ss, float look_ahead_dist, float &out_avoid_ang);

 private:
  float gsc = 0.0f;
  float sc = 0.0f;
  float scang = 0.0f;
  float ssp = 0.0f;
  float fsp = 0.0f;
  float sbpr = 0.0f;
};

typedef std::vector<Snake *> SnakeVec;
typedef std::unordered_map<snake_id_t, std::shared_ptr<Snake>> SnakeMap;
typedef SnakeMap::iterator SnakeMapIter;
typedef std::vector<snake_id_t> Ids;

#endif  // SRC_GAME_SNAKE_H_
