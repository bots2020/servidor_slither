/*
==================================================
FILE: world.h
RELATIVE PATH: game/world.h
==================================================
*/
#ifndef SRC_GAME_WORLD_H_
#define SRC_GAME_WORLD_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

#include "game/sector.h"
#include "game/snake.h"

class World {
 public:
  void Init(WorldConfig in_config);
  void InitSectors();
  void InitFood();

  void Tick(long dt);
  void UpdateServerSize(long dt);

  float GetGamu() const { return gamu; }
  float GetActiveRadius() const { return active_radius; }

  Snake::Ptr CreateSnake(int start_len = 0);
  Snake::Ptr CreateSnakeBot();
  void SpawnNumSnakes(const int count);
  void CheckSnakeBounds(Snake *s);

  void RegenerateFood(); 

  void InitRandom();
  int NextRandom();
  float NextRandomf();
  template <typename T>
  T NextRandom(T base);

  void AddSnake(Snake::Ptr ptr);
  void RemoveSnake(snake_id_t id);
  SnakeMapIter GetSnake(snake_id_t id);
  SnakeMap& GetSnakes();
  SectorSeq& GetSectors();
  Ids& GetDead();

  SnakeVec& GetChangedSnakes();

  void FlushChanges(snake_id_t id);
  void FlushChanges();

 private:
  void TickSnakes(long dt);
  
  // --- NEW: Helper to check for collisions before spawning ---
  bool IsLocationSafe(float x, float y, float safety_radius);

 private:
  SnakeMap snakes;
  Ids dead;
  SectorSeq sectors;
  SnakeVec changes;

  uint16_t lastSnakeId = 0;
  long ticks = 0;
  uint32_t frames = 0;
  float gamu = 1.0f;
  float active_radius = static_cast<float>(WorldConfig::game_radius);

  WorldConfig config;
};

std::ostream& operator<<(std::ostream& out, const World& w);

#endif  // SRC_GAME_WORLD_H_