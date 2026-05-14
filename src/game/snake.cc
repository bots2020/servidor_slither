#include "game/snake.h"
#include <iostream>
#include <array>
#include <algorithm> // For min/max
#include "game/math.h"

static inline float dist_sq(float x1, float y1, float x2, float y2) {
  const float dx = x1 - x2;
  const float dy = y1 - y2;
  return dx * dx + dy * dy;
}

// ----------------------------------------------------------------------
// MAIN TICK LOOP
// ----------------------------------------------------------------------

bool Snake::Tick(long dt, SectorSeq *ss, const WorldConfig &config) {
  uint8_t changes = 0;

  if (update & (change_dying | change_dead)) {
    return false;
  }

  // Battledome (anel verde no centro) é checado após mover a cabeça (fora do bloco AI/rotação).
  // Regra: entrar marca `in_battledome`; ao sair depois de ter entrado, cobra morre.
  
  // --- AI LOGIC ---
  if (bot) {
    ai_ticks += dt;
    if (ai_ticks > ai_step_interval) {
      const long frames = ai_ticks / ai_step_interval;
      const long frames_ticks = frames * ai_step_interval;

      // Pass the sectors to the AI so it can see food/enemies
      TickAI(frames, ss);

      ai_ticks -= frames_ticks;
    }
  }

  // --- ROTATION LOGIC ---
  if (angle != wangle) {
    rot_ticks += dt;
    if (rot_ticks >= rot_step_interval) {
      const long frames = rot_ticks / rot_step_interval;
      const long frames_ticks = frames * rot_step_interval;
      const float rotation = snake_angular_speed * frames_ticks / 1000.0f;
      float dAngle = Math::normalize_angle(wangle - angle);

      if (dAngle > Math::f_pi) {
        dAngle -= Math::f_2pi;
      }

      if (fabsf(dAngle) < rotation) {
        angle = wangle;
      } else {
        angle += rotation * (dAngle > 0 ? 1.0f : -1.0f);
      }

      angle = Math::normalize_angle(angle);

      changes |= change_angle;
      rot_ticks -= frames_ticks;
    }
  }

  // --- MOVEMENT LOGIC ---
  mov_ticks += dt;
  const long mov_frame_interval = 1000 * WorldConfig::move_step_distance / speed;
  if (mov_ticks >= mov_frame_interval) {
    const long frames = mov_ticks / mov_frame_interval;
    const long frames_ticks = frames * mov_frame_interval;
    const float move_dist = speed * frames_ticks / 1000.0f;
    const size_t len = parts.size();

    // move head
    Body &head = parts[0];
    Body prev = head;
    head.x += cosf(angle) * move_dist;
    head.y += sinf(angle) * move_dist;

    sbb.UpdateBoxNewSectors(ss, WorldConfig::sector_size / 2, head.x, head.y,
                            prev.x, prev.y);
    if (!bot) {
      vp.UpdateBoxNewSectors(ss, head.x, head.y, prev.x, prev.y);
    }

    // bound box
    float bbx = head.x;
    float bby = head.y;

    for (size_t i = 1; i < len && i < parts_skip_count; ++i) {
      const Body old = parts[i];
      parts[i] = prev;
      bbx += prev.x;
      bby += prev.y;
      prev = old;
    }

    // move intermediate
    for (size_t i = parts_skip_count, j = 0; i < len && i < parts_skip_count + parts_start_move_count; ++i) {
      Body &pt = parts[i];
      const Body last = parts[i - 1];
      const Body old = pt;

      pt.From(prev);
      const float move_coeff = snake_tail_k * (++j) / parts_start_move_count;
      pt.Offset(move_coeff * (last.x - pt.x), move_coeff * (last.y - pt.y));

      bbx += pt.x;
      bby += pt.y;
      prev = old;
    }

    // move tail
    for (size_t i = parts_skip_count + parts_start_move_count, j = 0; i < len; ++i) {
      Body &pt = parts[i];
      const Body last = parts[i - 1];
      const Body old = pt;

      pt.From(prev);
      pt.Offset(snake_tail_k * (last.x - pt.x), snake_tail_k * (last.y - pt.y));

      static const size_t tail_step =
          static_cast<size_t>(WorldConfig::sector_size / tail_step_distance);
      if (j + tail_step >= i) {
        sbb.UpdateBoxNewSectors(ss, WorldConfig::sector_size / 2, pt.x, pt.y,
                                old.x, old.y);
        j = i;
      }

      bbx += pt.x;
      bby += pt.y;
      prev = old;
    }

    changes |= change_pos;

    // update bb
    sbb.x = bbx / parts.size();
    sbb.y = bby / parts.size();
    vp.x = head.x;
    vp.y = head.y;
    UpdateBoxRadius();
    sbb.UpdateBoxOldSectors();
    if (!bot) {
      vp.UpdateBoxOldSectors();
    }
    
    // Check for food
    UpdateEatenFood(ss);

    // update speed
    if (acceleration) {
      // Check if we can afford to boost
      // Can only boost if size > start_score (or min_length if start_score is small)
      // Use target_score as the threshold (which is set to start_score)
      uint16_t threshold = target_score > 0 ? target_score : 10;
      
      // Allow boosting only if we are above the threshold
      if (parts.size() <= threshold && fullness == 0) {
        acceleration = false;
      } else {
        DecreaseSnake(config.boost_cost, config.boost_drop_size);
      }
    }

    const uint16_t wantedSpeed = acceleration ? boost_speed : base_move_speed;
    if (speed != wantedSpeed) {
      const float sgn = wantedSpeed > speed ? 1.0f : -1.0f;
      const uint16_t acc = static_cast<uint16_t>(speed_acceleration * frames_ticks / 1000.0f);
      if (abs(wantedSpeed - speed) <= acc) {
        speed = wantedSpeed;
      } else {
        speed += sgn * acc;
      }
      changes |= change_speed;
    }

    mov_ticks -= frames_ticks;
  }

  // Battledome check (após o movimento, usando a posição atual da cabeça).
  // Centro do mapa no sistema do servidor: (game_radius, game_radius).
  {
    const float cx = static_cast<float>(WorldConfig::game_radius); // centro X do mundo
    const float cy = static_cast<float>(WorldConfig::game_radius); // centro Y do mundo
    const float r = static_cast<float>(WorldConfig::battledome_radius); // raio do battledome

    const float head_x = parts[0].x;
    const float head_y = parts[0].y;

    const bool inside = dist_sq(head_x, head_y, cx, cy) <= (r * r);

    if (inside) {
      in_battledome = true; // entrou (ou continua dentro)
    } else {
      if (in_battledome) {
        // tentou sair depois de entrar => aplica mesma lógica de morte.
        // World::CheckSnakeBounds detecta morte via flags change_dying|change_dead.
        update |= change_dying;
        return false; // interrompe tick: cobra está “morrendo”
      }
    }
  }

  if (changes > 0 && changes != update) {
    update |= changes;
    return true;
  }

  return false;
}

std::shared_ptr<Snake> Snake::get_ptr() { return shared_from_this(); }

// ----------------------------------------------------------------------
// AI / BOT LOGIC
// ----------------------------------------------------------------------

// 1. Find Food (Scans 5x5 sectors)
void Snake::BotFindFood(SectorSeq *ss) {
    float hx = get_head_x();
    float hy = get_head_y();
    
    float best_x = (float)WorldConfig::game_radius;
    float best_y = (float)WorldConfig::game_radius;
    float max_score = -1.0f;

    int16_t center_sx = static_cast<int16_t>(hx / WorldConfig::sector_size);
    int16_t center_sy = static_cast<int16_t>(hy / WorldConfig::sector_size);

    // Calculate minimum turning radius based on speed
    // This prevents the "Spinning" behavior.
    // If food is inside this radius and requires a hard turn, ignore it.
    float turn_radius = (speed * 0.033f) / snake_angular_speed; 
    float min_safe_dist_sq = turn_radius * turn_radius;

    for (int16_t sy = center_sy - 2; sy <= center_sy + 2; ++sy) {
        for (int16_t sx = center_sx - 2; sx <= center_sx + 2; ++sx) {
            if (sx < 0 || sx >= WorldConfig::sector_count_along_edge ||
                sy < 0 || sy >= WorldConfig::sector_count_along_edge) continue;

            Sector *sec = ss->get_sector(sx, sy);
            
            for (const Food &f : sec->food) {
                float dist_sq = Math::dist_sq(hx, hy, f.x, f.y);
                
                // ---------------------------------------------------------
                // FIX 2: Prevent Spinning
                // ---------------------------------------------------------
                // If food is too close...
                if (dist_sq < min_safe_dist_sq) {
                    // Check angle difference
                    float ang_to_food = atan2f(f.y - hy, f.x - hx);
                    float angle_diff = fabs(Math::normalize_angle(ang_to_food - angle));
                    
                    // If we have to turn more than 45 degrees (PI/4) to hit something 
                    // that is inside our turn radius, we will orbit it forever.
                    // Ignore it so we straighten out and loop back later.
                    if (angle_diff > (Math::f_pi / 4.0f)) {
                        continue; 
                    }
                }
                // ---------------------------------------------------------

                float score = (f.size * f.size) / (dist_sq + 1.0f); // +1 to avoid div0

                if (score > max_score) {
                    max_score = score;
                    best_x = f.x;
                    best_y = f.y;
                }
            }
        }
    }

    bot_target_x = best_x;
    bot_target_y = best_y;
    
    // Only boost if the food is worth it AND it's reasonably far away to control the speed
    if (fullness > 30 && max_score > 0.05f) {
        acceleration = true;
    } else {
        acceleration = false;
    }
}

// 2. Check Collision (Projects whisker)
bool Snake::BotCheckCollision(SectorSeq *ss, float look_ahead_dist, float &out_avoid_ang) {
    float hx = get_head_x();
    float hy = get_head_y();
    
    // Look ahead point
    float whisker_x = hx + cosf(angle) * look_ahead_dist;
    float whisker_y = hy + sinf(angle) * look_ahead_dist;

    // A. Check World Map Bounds
    float gr = (float)WorldConfig::game_radius;
    if (Math::dist_sq(whisker_x, whisker_y, gr, gr) >= WorldConfig::death_radius * WorldConfig::death_radius) {
        // Turn towards center
        out_avoid_ang = atan2f(gr - hy, gr - hx);
        return true;
    }

    // B. Check Snake Collisions
    int16_t sx = static_cast<int16_t>(whisker_x / WorldConfig::sector_size);
    int16_t sy = static_cast<int16_t>(whisker_y / WorldConfig::sector_size);
    
    // Check 3x3 around whisker tip
    for (int j = sy - 1; j <= sy + 1; j++) {
        for (int i = sx - 1; i <= sx + 1; i++) {
            if (i < 0 || i >= WorldConfig::sector_count_along_edge || 
                j < 0 || j >= WorldConfig::sector_count_along_edge) continue;

            Sector *sec = ss->get_sector(i, j);

            for (const BoundBox *bb : sec->snakes) {
                const Snake *other = bb->snake;
                if (other == this) continue;

                // Optimization: Bounding box check first
                if (fabs(whisker_x - other->sbb.x) > other->sbb.r + 50) continue;
                if (fabs(whisker_y - other->sbb.y) > other->sbb.r + 50) continue;

                // Check Body Parts (Simplified Circle Check for speed)
                float collision_dist_sq = (sbpr + other->sbpr + 40.0f); // Buffer 40 units
                collision_dist_sq *= collision_dist_sq;

                for (const Body &b : other->parts) {
                    if (Math::dist_sq(whisker_x, whisker_y, b.x, b.y) < collision_dist_sq) {
                        
                        // Collision detected! Decide turn direction.
                        float ang_to_obs = atan2f(b.y - hy, b.x - hx);
                        float rel_ang = Math::normalize_angle(ang_to_obs - angle);

                        // If obstacle is to our left, turn right. Else turn left.
                        if (rel_ang > 0) {
                            out_avoid_ang = angle - (Math::f_pi / 1.5f); 
                        } else {
                            out_avoid_ang = angle + (Math::f_pi / 1.5f); 
                        }
                        
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// 3. AI Main Logic
void Snake::TickAI(long frames, SectorSeq *ss) {
    // 1. Find Food Target (Goal)
    BotFindFood(ss);

    // 2. Default Wanted Angle: Towards Food
    float target_ang = atan2f(bot_target_y - get_head_y(), bot_target_x - get_head_x());

    // 3. Collision Avoidance (Override)
    // Look ahead 3x the snake width + some speed factor
    float look_ahead = (lsz * 4.0f) + (speed * 0.4f); 
    float avoid_ang = 0;

    if (BotCheckCollision(ss, look_ahead, avoid_ang)) {
        target_ang = avoid_ang;
        acceleration = false; // Stop boosting if in danger
    }

    // 4. Set Rotation
    wangle = Math::normalize_angle(target_ang);
    update |= change_wangle;
}

// ----------------------------------------------------------------------
// STANDARD SNAKE FUNCTIONS
// ----------------------------------------------------------------------

BoundBox Snake::get_new_box() const {
  return {{get_head_x(), get_head_y(), 0}, id, this, {}};
}

void Snake::UpdateBoxCenter() {
  float x = 0.0f;
  float y = 0.0f;
  for (const Body &p : parts) {
    x += p.x;
    y += p.y;
  }
  x /= parts.size();
  y /= parts.size();
  sbb.x = x;
  sbb.y = y;
  vp.x = get_head_x();
  vp.y = get_head_y();
}

void Snake::UpdateBoxRadius() {
  // calculate bb radius, len eval for step dist = 42, k = 0.43
  // parts ..  1  ..  2  ..  3   ..  4   ..  5   ..  6   ..  7  .. tail by 24.0f
  float d = 42.0f + 42.0f + 42.0f + 37.7f + 37.7f + 33.0f + 28.5f;

  if (parts.size() > 8) {
    d += tail_step_distance * (parts.size() - 8);
  }

  // reserve 1 step ahead of the snake radius
  sbb.r = (d + WorldConfig::move_step_distance) / 2.0f;
  vp.r = WorldConfig::sector_diag_size * 3.0f;
}

// UPDATED: AS3 Physics Constants
void Snake::UpdateSnakeConsts() {
  float sct = (float)parts.size();
  
  // Client: sc = Math.min(6, 1 + (sct - 2) / 106);
  sc = fminf(6.0f, 1.0f + (sct - 2.0f) / 106.0f);

  // Client: sc13 = Math.pow(sc, 1.3);
  sc13 = powf(sc, 1.3f);
  
  // Client: lsz = 29 * sc;
  lsz = 29.0f * sc;

  // gsc calculation
  gsc = 0.5f + 0.4f / fmaxf(1.0f, (sct + 16.0f) / 36.0f);

  // scang calculation
  const float scang_x = (7.0f - sc) / 6.0f;
  scang = 0.13f + 0.87f * scang_x * scang_x;

  ssp = nsp1 + nsp2 * sc;
  fsp = ssp + 0.1f;

  sbpr = lsz * 0.5f; 
}

void Snake::InitBoxNewSectors(SectorSeq *ss) {
  Body &head = parts[0];
  sbb.UpdateBoxNewSectors(ss, WorldConfig::sector_size / 2, head.x, head.y,
                          0.0f, 0.0f);

  if (!bot) {
    vp.UpdateBoxNewSectors(ss, head.x, head.y, 0.0f, 0.0f);
  }

  const size_t len = parts.size();
  // as far as having step dist = 42, k = 0.43, sec. size = 300, this could be
  // 300 / 24.0f, with radius 150
  static const size_t tail_step = static_cast<size_t>(WorldConfig::sector_size / tail_step_distance);
  for (size_t i = 3; i < len; i += tail_step) {
    Body &pt = parts[i];
    sbb.UpdateBoxNewSectors(ss, WorldConfig::sector_size / 2, pt.x, pt.y, 0.0f,
                            0.0f);
  }
}

// UPDATED: 3x3 Sector Scan for Food
void Snake::UpdateEatenFood(SectorSeq *ss) {
  float head_x = get_head_x();
  float head_y = get_head_y();

  // Project mouth position forward based on speed and size
  float client_sp = speed / 32.0f; // Approx conversion
  float dist_offset = (0.36f * lsz + 31.0f) * (client_sp / 4.8f);

  float mouth_x = head_x + cosf(angle) * dist_offset;
  float mouth_y = head_y + sinf(angle) * dist_offset;

  // Calculate Eat Radius Squared (AS3: dcsc = 1600 * sc13)
  // FIX: Increased radius slightly to ensure food is consumed reliably
  float eat_dist_sq = 2000.0f * sc13;
  float search_r = sqrtf(eat_dist_sq) + 40.0f; 

  int16_t center_sx = static_cast<int16_t>(mouth_x / WorldConfig::sector_size);
  int16_t center_sy = static_cast<int16_t>(mouth_y / WorldConfig::sector_size);

  // Scan 3x3 Neighbors
  for (int16_t sy = center_sy - 1; sy <= center_sy + 1; ++sy) {
    for (int16_t sx = center_sx - 1; sx <= center_sx + 1; ++sx) {
        
        if (sx < 0 || sx >= WorldConfig::sector_count_along_edge ||
            sy < 0 || sy >= WorldConfig::sector_count_along_edge) continue;

        Sector *sec = ss->get_sector(sx, sy);
        
        auto it = sec->food.begin();
        while (it != sec->food.end()) {
            // Fast Bounding Box Check
            if (fabs(it->x - mouth_x) < search_r && fabs(it->y - mouth_y) < search_r) {
                // Exact Distance Check
                if (Math::dist_sq(it->x, it->y, mouth_x, mouth_y) < eat_dist_sq) {
                    on_food_eaten(*it);
                    it = sec->food.erase(it);
                    continue; 
                }
            }
            ++it;
        }
    }
  }
}

// Legacy Intersect (Required by World::CheckSnakeBounds logic for optimization)
// Actual collision logic is in World::CheckSnakeBounds
bool Snake::Intersect(BoundBoxPos foe) const {
    float hx = get_head_x();
    float hy = get_head_y();
    // Simple circle check
    float r_sum = sbpr + foe.r;
    return Math::dist_sq(hx, hy, foe.x, foe.y) < r_sum * r_sum;
}

void Snake::on_food_eaten(Food f) {
  IncreaseSnake(f.size);
  eaten.push_back({f.x, f.y, f.size, f.color});
}

void Snake::IncreaseSnake(uint16_t volume) {
  fullness += volume;
  if (fullness >= 100) {
    fullness -= 100;
    parts.push_back(parts.back());
  }
  update |= change_fullness;
  UpdateSnakeConsts();
}

void Snake::DecreaseSnake(uint16_t volume, uint8_t drop_size) {
  if (volume > fullness) {
    volume -= fullness;
    const uint16_t reduce = static_cast<uint16_t>(1 + volume / 100);
    for (uint16_t i = 0; i < reduce; i++) {
      if (parts.size() > 3) {
        const Body &last = parts.back();
        SpawnFood({static_cast<uint16_t>(last.x),
                   static_cast<uint16_t>(last.y),
                   drop_size,
                   skin});
        parts.pop_back();
        
        // Safety: Check if we hit the limit during decrease
        uint16_t threshold = target_score > 0 ? target_score : 10;
        if (parts.size() <= threshold) break;
      }
    }
    fullness = static_cast<uint16_t>(100 - volume % 100);
  } else {
    fullness -= volume;
  }
  update |= change_fullness;
  UpdateSnakeConsts();
}

void Snake::SpawnFood(Food f) {
  const int16_t sx = static_cast<int16_t>(f.x / WorldConfig::sector_size);
  const int16_t sy = static_cast<int16_t>(f.y / WorldConfig::sector_size);

  for (Sector *sec : sbb.sectors) {
    if (sec->x == sx && sec->y == sy) {
      sec->Insert(f);
      spawn.push_back(f);
      break;
    }
  }
}

void Snake::on_dead_food_spawn(SectorSeq *ss, std::function<float()> next_randomf) {
  auto end = parts.end();
  const float r = get_snake_body_part_radius();
  const uint16_t r2 = static_cast<uint16_t>(r * 3);
  const size_t count = static_cast<size_t>(sc * 2);
  const uint8_t food_size = static_cast<uint8_t>(100 / count);

  for (auto i = parts.begin(); i != end; ++i) {
    // Safety check for NaN or negative coordinates
    if (std::isnan(i->x) || std::isnan(i->y) || i->x < 0 || i->y < 0) continue;

    const uint16_t sx = static_cast<uint16_t>(i->x / WorldConfig::sector_size);
    const uint16_t sy = static_cast<uint16_t>(i->y / WorldConfig::sector_size);
    
    // Bounds check before accessing sector array
    if (sx < WorldConfig::sector_count_along_edge && 
        sy < WorldConfig::sector_count_along_edge) {
      
      for (size_t j = 0; j < count; j++) {
        Food f = {static_cast<uint16_t>(i->x + r - next_randomf() * r2),
                  static_cast<uint16_t>(i->y + r - next_randomf() * r2),
                  food_size, static_cast<uint8_t>(29 * next_randomf())};

        // Double check food bounds
        if (f.x < WorldConfig::game_radius * 2 && f.y < WorldConfig::game_radius * 2) {
            Sector *sec = ss->get_sector(sx, sy);
            if (sec) {
                sec->Insert(f);
                spawn.push_back(f);
            }
        }
      }
    }
  }
}

float Snake::get_snake_scale() const { return gsc; }
float Snake::get_snake_body_part_radius() const { return sbpr; }

// Score Calculation
std::array<float, WorldConfig::max_snake_parts> get_fmlts() {
  std::array<float, WorldConfig::max_snake_parts> data = {{0.0f}};
  for (size_t i = 0; i < data.size(); i++) {
    data[i] = powf(1.0f - 1.0f * i / data.size(), 2.25f);
  }
  return data;
}

std::array<float, WorldConfig::max_snake_parts> get_fpsls(
    const std::array<float, WorldConfig::max_snake_parts> &fmlts) {
  std::array<float, WorldConfig::max_snake_parts> data = {{0.0f}};
  for (size_t i = 1; i < data.size(); i++) {
    data[i] = data[i - 1] + 1.0f / fmlts[i - 1];
  }
  return data;
}

uint16_t Snake::get_snake_score() const {
  static std::array<float, WorldConfig::max_snake_parts> fmlts = get_fmlts();
  static std::array<float, WorldConfig::max_snake_parts> fpsls = get_fpsls(fmlts);

  // FIX: Use parts.size() as sct (length), not parts.size() - 1
  // Protocol: sct is snake body parts count (length) taking values between [2 .. mscps]
  size_t sct = parts.size();
  
  // FIX: Use target_score (length) if it's larger than current length
  // This ensures the leaderboard shows the "intended" score during spawn animation
  if (target_score > 0 && sct < target_score) {
      sct = target_score;
  }

  if (sct >= fmlts.size()) {
    sct = fmlts.size() - 1;
  }

  // Formula: Math.floor(15 * (fpsls[snake.sct] + snake.fam / fmlts[snake.sct] - 1) - 5) / 1
  // snake.fam is fullness / 100.0f (0..1)
  return static_cast<uint16_t>(15.0f * (fpsls[sct] + (fullness / 100.0f) / fmlts[sct] - 1) - 5);
}
