/*
==================================================
FILE: config.h
RELATIVE PATH: game/config.h
==================================================
*/
#ifndef SRC_GAME_CONFIG_H_
#define SRC_GAME_CONFIG_H_

#include <cstdint>

typedef uint16_t snake_id_t;

struct WorldConfig {
  // generate bots on start count
  uint16_t bots = 0;
  
  // Bot spawning
  bool bot_respawn = true;

  // Smooth Food Spawn Rate
  uint16_t food_spawn_rate = 2; 

  // --- NEW: Food Location Targeting Weights ---
  // These work as ratios. e.g., 25, 25, 50 = 25% near, 25% on, 50% random.
  uint16_t spawn_prob_near_snake = 25; // 1. Chance to target neighbor of a snake
  uint16_t spawn_prob_on_snake = 25;   // 2. Chance to target current sector of a snake
  uint16_t spawn_prob_random = 50;     // 3. Chance to target completely random sector
  // -------------------------------------------

  // Snake Score/Length Settings
  // FIXED: Lowered drastically. 
  // Length 2 = Score 0. Length 5 = Score ~25.
  uint16_t h_snake_start_score = 5; 
  uint16_t b_snake_start_score = 5;
  
  // Start tiny (2 parts) to allow the "growing" spawn animation to play
  uint16_t snake_min_length = 2;

  // Boost Settings
  uint16_t boost_cost = 20;
  uint8_t boost_drop_size = 10;

  // Original Slither.io values
  static const uint16_t game_radius = 3584;
  static const uint16_t max_snake_parts = 411;

  static const uint16_t sector_size = 256; 
  static const uint16_t sector_count_along_edge = 28; 
  
  static const uint16_t dome_radius = 2100;
  static const uint16_t death_radius = game_radius;
  static const uint16_t sector_diag_size = 362; 
  static const uint16_t move_step_distance = 42; 
  
  static const long frame_time_ms = 8;
  static const uint8_t protocol_version = 14;
};

#endif  // SRC_GAME_CONFIG_H_