/*
==================================================
FILE: world.cc
RELATIVE PATH: game/world.cc
==================================================
*/
#include "game/world.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <vector>

#include "game/math.h"
#include "game/bot_names.h" 

// --- NEW: Safety Check Implementation ---
bool World::IsLocationSafe(float x, float y, float safety_radius) {
    int16_t sx = static_cast<int16_t>(x / WorldConfig::sector_size);
    int16_t sy = static_cast<int16_t>(y / WorldConfig::sector_size);
    
    float safe_sq = safety_radius * safety_radius;

    // Check 3x3 sectors around the spawn point
    for (int j = sy - 1; j <= sy + 1; j++) {
        for (int i = sx - 1; i <= sx + 1; i++) {
            // Boundary checks for sectors
            if (i < 0 || i >= WorldConfig::sector_count_along_edge || 
                j < 0 || j >= WorldConfig::sector_count_along_edge) continue;

            Sector *sec = sectors.get_sector(i, j);

            // Iterate over all snakes currently in this sector
            for (const BoundBox *bb : sec->snakes) {
                const Snake *other = bb->snake;
                
                // Check distance to the other snake's head
                if (Math::dist_sq(x, y, other->get_head_x(), other->get_head_y()) < safe_sq) {
                    return false; // Too close!
                }

                // Optional: Check distance to *any* body part if you want to be super safe
                // but checking the head is usually sufficient for spawning.
            }
        }
    }
    return true;
}

Snake::Ptr World::CreateSnake(int start_len) {
  lastSnakeId++;

  auto s = std::make_shared<Snake>();
  s->id = lastSnakeId;
  s->name = "";
  s->skin = static_cast<uint8_t>(9 + NextRandom(21 - 9 + 1));
  s->speed = Snake::base_move_speed;
  s->fullness = 0;

  // --- IMPROVED SPAWN LOGIC ---
  float angle;
  float dist;
  uint16_t x, y;
  
  // Try up to 20 times to find a safe spot
  // If the server is super crowded, we give up and spawn at the last generated point
  int attempts = 0;
  bool safe = false;
  
  const float max_spawn_radius = active_radius - 300.0f;
  const float safety_buffer = 150.0f;
  const float dome_r = static_cast<float>(WorldConfig::dome_radius);
  const float min_spawn = dome_r + 100.0f;

  while (attempts < 20) {
      angle = Math::f_2pi * NextRandomf();

      float random_factor = sqrt(NextRandomf());
      const float min_dist = std::min(min_spawn, max_spawn_radius * 0.98f);
      dist = min_dist + random_factor * (max_spawn_radius - min_dist);
      
      float fx = WorldConfig::game_radius + dist * cosf(angle);
      float fy = WorldConfig::game_radius + dist * sinf(angle);
      
      if (IsLocationSafe(fx, fy, safety_buffer)) {
          x = static_cast<uint16_t>(fx);
          y = static_cast<uint16_t>(fy);
          safe = true;
          break;
      }
      attempts++;
  }

  // Fallback: If map is full, just calculate based on last attempt
  if (!safe) {
      x = static_cast<uint16_t>(WorldConfig::game_radius + dist * cosf(angle));
      y = static_cast<uint16_t>(WorldConfig::game_radius + dist * sinf(angle));
  }
  
  // Point snake towards the center of the map initially, 
  // so they don't immediately drive into the wall
  float angle_to_center = atan2f(WorldConfig::game_radius - y, WorldConfig::game_radius - x);
  // Add a little randomness to the angle (-45 to +45 degrees)
  angle = angle_to_center + (NextRandomf() * 1.5f - 0.75f);
  
  // Normalize
  angle = Math::normalize_angle(angle);

  // -----------------------------

  // Fix ensure spawn length is valid
  const int len = config.snake_min_length;
  uint16_t intended_score = (start_len > 0) ? start_len : config.h_snake_start_score;
  if (intended_score < len) intended_score = len;
  s->target_score = intended_score;

  // Generate body parts backwards from head
  for (int i = 0; i < len && i < Snake::parts_skip_count + Snake::parts_start_move_count; ++i) {
    s->parts.push_back(Body{1.0f * x, 1.0f * y});
    // Move "backwards" to place tail
    x -= cosf(angle) * WorldConfig::move_step_distance;
    y -= sinf(angle) * WorldConfig::move_step_distance;
  }

  for (int i = Snake::parts_skip_count + Snake::parts_start_move_count; i < len; ++i) {
    s->parts.push_back(Body{1.0f * x, 1.0f * y});
    x -= cosf(angle) * Snake::tail_step_distance;
    y -= sinf(angle) * Snake::tail_step_distance;
  }

  s->clientPartsIndex = s->parts.size();
  s->angle = Math::normalize_angle(angle);
  s->wangle = Math::normalize_angle(angle);
  s->dome_grow_ticks = Snake::dome_grow_interval;

  s->sbb = SnakeBoundBox(s->get_new_box());
  s->vp = ViewPort(s->get_new_box());
  s->UpdateBoxCenter();
  s->UpdateBoxRadius();
  s->UpdateSnakeConsts();
  s->InitBoxNewSectors(&sectors);

  return s;
}

Snake::Ptr World::CreateSnakeBot() {
  Snake::Ptr ptr = CreateSnake(config.b_snake_start_score);
  ptr->bot = true;

  if (!BOT_NAMES.empty()) {
      int name_idx = NextRandom(static_cast<int>(BOT_NAMES.size()));
      // FIX: Appending (Bot) tag here
      ptr->name = BOT_NAMES[name_idx] + " (Bot)";
  } else {
      ptr->name = "Unnamed Bot";
  }
  
  // Debug output using cerr (flushes immediately so you see it even if it crashes later)
  std::cerr << "[WORLD] Created Bot: " << ptr->name << " (ID: " << ptr->id << ")" << std::endl;
  
  return ptr;
}

void World::InitRandom() { std::srand(std::time(nullptr)); }

int World::NextRandom() { return std::rand(); }

float World::NextRandomf() { return 1.0f * std::rand() / RAND_MAX; }

template <typename T>
T World::NextRandom(T base) {
  return static_cast<T>(NextRandom() % base);
}

void World::UpdateServerSize(long dt) {
  static const float MIN_GAMU    = 1.0f;
  static const float CHANGE_RATE = 0.000008f;

  uint32_t total_food = 0;
  uint32_t max_food   = 0;
  for (const auto &s : sectors) {
    total_food += static_cast<uint32_t>(s.food.size());
    max_food   += static_cast<uint32_t>(s.max_food_capacity);
  }
  if (max_food == 0) max_food = 1;

  const float food_frac  = static_cast<float>(total_food) / max_food;
  const float snake_frac = std::min(1.0f, static_cast<float>(snakes.size()) / 50.0f);
  const float target_gamu = std::max(MIN_GAMU, std::min(1.0f, 0.7f * snake_frac + 0.3f * food_frac));

  if (gamu < target_gamu) {
    gamu = std::min(target_gamu, gamu + CHANGE_RATE * dt);
  } else if (gamu > target_gamu) {
    gamu = std::max(target_gamu, gamu - CHANGE_RATE * dt);
  }

  active_radius = WorldConfig::game_radius * gamu;
}

void World::Tick(long dt) {
  ticks += dt;
  const long vfr = ticks / WorldConfig::frame_time_ms;
  if (vfr > 0) {
    const long vfr_time = vfr * WorldConfig::frame_time_ms;
    UpdateServerSize(vfr_time);
    TickSnakes(vfr_time);
    RegenerateFood();
    ticks -= vfr_time;
    frames += vfr;
  }
}

void World::TickSnakes(long dt) {
  for (auto pair : snakes) {
    Snake *const s = pair.second.get();

    if (s->Tick(dt, &sectors, config)) {
      changes.push_back(s);
    }
  }

  for (auto s : changes) {
    if (s->update & change_pos) {
      CheckSnakeBounds(s);
    }
  }
}

void World::RegenerateFood() {
    // 1. Calculate Total Probability Weight
    uint32_t w_near = config.spawn_prob_near_snake;
    uint32_t w_on   = config.spawn_prob_on_snake;
    uint32_t w_rand = config.spawn_prob_random;
    uint32_t total_weight = w_near + w_on + w_rand;

    // Safety check to prevent divide by zero
    if (total_weight == 0) total_weight = 1;

    for (int i = 0; i < config.food_spawn_rate; i++) {
        Sector* target_sector = nullptr;
        
        // Roll the dice (0 to total_weight - 1)
        uint32_t roll = NextRandom(total_weight);

        // --- OPTION A: Target an Existing Snake (Near or On) ---
        // We only do this if the roll is within the snake weights AND there are snakes alive
        if (roll < (w_near + w_on) && !snakes.empty()) {
             auto it = snakes.begin();
             // Select a random snake
             std::advance(it, NextRandom(snakes.size()));
             Snake* s = it->second.get();
             
             int16_t sx = static_cast<int16_t>(s->get_head_x() / WorldConfig::sector_size);
             int16_t sy = static_cast<int16_t>(s->get_head_y() / WorldConfig::sector_size);
             
             // Sub-selection: Neighbor vs Current
             if (roll < w_near) {
                 // 1. Target Neighbor Sector (Near Snake)
                 // Pick a random offset (-1, 0, or 1)
                 sx += (NextRandom(3) - 1);
                 sy += (NextRandom(3) - 1);
             } 
             // Else: 2. Target Current Sector (On Snake) -> Keep sx, sy as is

             // Validate Map Bounds
             if (sx >= 0 && sx < WorldConfig::sector_count_along_edge &&
                 sy >= 0 && sy < WorldConfig::sector_count_along_edge) {
                 target_sector = sectors.get_sector(sx, sy);
             }
        }

        // --- OPTION B: Random Sector ---
        // 3. Fallback to random if roll was high OR if targeting failed (e.g., bounds error)
        if (target_sector == nullptr) {
            int sec_idx = NextRandom(sectors.size());
            target_sector = &sectors[sec_idx];
        }
        
        // --- CAPACITY CHECK ---
        // If this sector is already overfilled, skip this specific spawn attempt.
        if (target_sector->food.size() >= target_sector->max_food_capacity) {
            continue; 
        }

        // Generate position within the chosen sector
        uint16_t fx = target_sector->x * WorldConfig::sector_size + NextRandom<uint16_t>(WorldConfig::sector_size);
        uint16_t fy = target_sector->y * WorldConfig::sector_size + NextRandom<uint16_t>(WorldConfig::sector_size);
        
        const float dome_r = static_cast<float>(WorldConfig::dome_radius);
        float d_sq = Math::dist_sq(fx, fy, (float)WorldConfig::game_radius, (float)WorldConfig::game_radius);
        if (d_sq >= dome_r * dome_r) continue;

        // Insert the food
        target_sector->Insert(Food{
            fx, fy, 
            static_cast<uint8_t>(5 + NextRandom<uint8_t>(11)), 
            static_cast<uint8_t>(NextRandom<uint8_t>(29))
        });
    }
}

void World::CheckSnakeBounds(Snake *s) {
  static std::vector<snake_id_t> cs_cache;
  cs_cache.clear();

  float hx = s->get_head_x();
  float hy = s->get_head_y();
  
  float move_dist = s->speed * WorldConfig::frame_time_ms / 1000.0f;
  if (move_dist < 5.0f) move_dist = 5.0f;

  float prev_hx = hx - cosf(s->angle) * move_dist;
  float prev_hy = hy - sinf(s->angle) * move_dist;

  float body_radius = s->lsz / 2.0f;
  float tip_x = hx + cosf(s->angle) * body_radius;
  float tip_y = hy + sinf(s->angle) * body_radius;

  const float gr = static_cast<float>(WorldConfig::game_radius);
  const float dome_r = static_cast<float>(WorldConfig::dome_radius);
  const float dist_sq_center = Math::dist_sq(hx, hy, gr, gr);

  if (!s->dome_entered) {
    if (dist_sq_center < dome_r * dome_r) {
      s->dome_entered = true;
    }
  } else {
    if (dist_sq_center >= dome_r * dome_r) {
      s->update |= change_dying;
      return;
    }
  }

  const float dth = active_radius - WorldConfig::sector_size;
  if (Math::dist_sq(tip_x, tip_y, gr, gr) >= dth * dth) {
    s->update |= change_dying;
    return;
  }

  int16_t sx = static_cast<int16_t>(hx / WorldConfig::sector_size);
  int16_t sy = static_cast<int16_t>(hy / WorldConfig::sector_size);

  for (int j = sy - 1; j <= sy + 1; j++) {
    for (int i = sx - 1; i <= sx + 1; i++) {
      if (i < 0 || i >= WorldConfig::sector_count_along_edge || 
          j < 0 || j >= WorldConfig::sector_count_along_edge) continue;

      Sector *sec_ptr = sectors.get_sector(i, j);

      for (const BoundBox *bb : sec_ptr->snakes) {
        const Snake *other = bb->snake;
        
        if (other == s || (other->update & (change_dying|change_dead))) continue; // Skip dying snakes
        
        bool checked = false;
        for(auto id : cs_cache) if(id == other->id) { checked = true; break; }
        if(checked) continue;
        cs_cache.push_back(other->id);

        if (!s->sbb.Intersect(other->sbb)) continue;

        float hit_r = (s->lsz/2.0f) + (other->lsz/2.0f);
        float hit_dist_sq = hit_r * hit_r;

        size_t len = other->parts.size();
        if (len < 2) continue;

        for (size_t k = 0; k < len - 1; ++k) {
             const Body &b1 = other->parts[k];
             const Body &b2 = other->parts[k+1];

             if (Math::dist_sq(hx, hy, b1.x, b1.y) < hit_dist_sq) {
                 s->update |= change_dying;
                 return;
             }

             if (Math::check_intersection(prev_hx, prev_hy, hx, hy, 
                                          b1.x, b1.y, b2.x, b2.y)) {
                 s->update |= change_dying;
                 return;
             }
        }
        
        const Body &last = other->parts.back();
        if (Math::dist_sq(hx, hy, last.x, last.y) < hit_dist_sq) {
            s->update |= change_dying;
            return;
        }
      }
    }
  }
}

void World::Init(WorldConfig in_config) {
  config = in_config;

  InitRandom();
  InitSectors();
  InitFood();

  SpawnNumSnakes(in_config.bots);
}

void World::InitSectors() {
  sectors.InitSectors();
}

void World::InitFood() {
  const float gr = static_cast<float>(WorldConfig::game_radius);
  const float dome_r = static_cast<float>(WorldConfig::dome_radius);

  for (Sector &s : sectors) {
    const uint8_t cx = WorldConfig::sector_count_along_edge / 2;
    const uint8_t cy = cx;
    const uint16_t dist = (s.x - cx) * (s.x - cx) + (s.y - cy) * (s.y - cy);
    const float dp = 1.0f -
                     1.0f * dist / (WorldConfig::sector_count_along_edge *
                                    WorldConfig::sector_count_along_edge);

    const float scx = s.x * WorldConfig::sector_size + WorldConfig::sector_size * 0.5f;
    const float scy = s.y * WorldConfig::sector_size + WorldConfig::sector_size * 0.5f;
    const bool inside_dome = Math::dist_sq(scx, scy, gr, gr) < dome_r * dome_r;

    if (!inside_dome) {
      s.max_food_capacity = 0;
      continue;
    }

    const size_t density = static_cast<size_t>(dp * 2);
    s.max_food_capacity = std::max((size_t)4, density * 2);

    for (size_t i = 0; i < density; i++) {
      s.Insert(
          Food{static_cast<uint16_t>(
                   s.x * WorldConfig::sector_size +
                       NextRandom<uint16_t>(WorldConfig::sector_size)),
               static_cast<uint16_t>(
                   s.y * WorldConfig::sector_size +
                       NextRandom<uint16_t>(WorldConfig::sector_size)),
               static_cast<uint8_t>(5 + NextRandom<uint8_t>(11)),
               NextRandom<uint8_t>(29)});
    }
    s.Sort();
  }

  const float dr = dome_r;
  const float arc_step = 15.0f;
  const int ring_count = static_cast<int>(Math::f_2pi * dr / arc_step);

  for (int i = 0; i < ring_count; i++) {
    float theta = Math::f_2pi * i / ring_count;
    uint16_t rx = static_cast<uint16_t>(gr + dr * cosf(theta));
    uint16_t ry = static_cast<uint16_t>(gr + dr * sinf(theta));

    uint16_t fsx = rx / WorldConfig::sector_size;
    uint16_t fsy = ry / WorldConfig::sector_size;

    if (fsx >= WorldConfig::sector_count_along_edge ||
        fsy >= WorldConfig::sector_count_along_edge) continue;

    Sector *sec = sectors.get_sector(fsx, fsy);
    if (sec) sec->Insert(Food{rx, ry, 10, 14, true});
  }
}

void World::AddSnake(Snake::Ptr ptr) {
  snakes.insert({ptr->id, ptr});
}

void World::RemoveSnake(snake_id_t id) {
  FlushChanges(id);

  auto sn_i = GetSnake(id);
  if (sn_i != snakes.end()) {
    // Redundant: ~BoundBox handles this now
    /*
    for (auto sec_ptr : sn_i->second->sbb.sectors) {
      sec_ptr->RemoveSnake(id);
    }
    */

    snakes.erase(id);
  }
}

SnakeMapIter World::GetSnake(snake_id_t id) {
  return snakes.find(id);
}

SnakeMap &World::GetSnakes() { return snakes; }

Ids &World::GetDead() { return dead; }

std::vector<Snake *> &World::GetChangedSnakes() { return changes; }

void World::FlushChanges() { changes.clear(); }

void World::FlushChanges(snake_id_t id) {
  changes.erase(
      std::remove_if(changes.begin(), changes.end(),
                     [id](const Snake *s) { return s->id == id; }), changes.end());
}

void World::SpawnNumSnakes(const int count) {
  for (int i = 0; i < count; i++) {
    AddSnake(CreateSnakeBot());
  }
}

SectorSeq &World::GetSectors() { return sectors; }

std::ostream &operator<<(std::ostream &out, const World &w) {
  return out << "\tgame_radius = " << WorldConfig::game_radius
             << "\n\tmax_snake_parts = " << WorldConfig::max_snake_parts
             << "\n\tsector_size = " << WorldConfig::sector_size
             << "\n\tsector_count_along_edge = " << WorldConfig::sector_count_along_edge
             << "\n\tvirtual_frame_time_ms = " << WorldConfig::frame_time_ms
             << "\n\tprotocol_version = " << static_cast<long>(WorldConfig::protocol_version)
             << "\n\tspangdv = " << Snake::spangdv
             << "\n\tnsp1 = " << Snake::nsp1 << "\n\tnsp2 = " << Snake::nsp2
             << "\n\tnsp3 = " << Snake::nsp3
             << "\n\tbase_move_speed = " << Snake::base_move_speed
             << "\n\tboost_speed = " << Snake::boost_speed
             << "\n\tspeed_acceleration = " << Snake::speed_acceleration
             << "\n\tprey_angular_speed = " << Snake::prey_angular_speed
             << "\n\tsnake_angular_speed = " << Snake::snake_angular_speed
             << "\n\tsnake_tail_k = " << Snake::snake_tail_k
             << "\n\tparts_skip_count = " << Snake::parts_skip_count
             << "\n\tparts_start_move_count = " << Snake::parts_start_move_count
             << "\n\tmove_step_distance = " << WorldConfig::move_step_distance
             << "\n\trot_step_angle = " << Snake::rot_step_angle;
}