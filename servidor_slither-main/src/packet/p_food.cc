#include "packet/p_food.h"
#include "server/config.h" 

static void get_sector_coords(uint16_t world_val, uint8_t& sector, uint8_t& rel) {
    uint16_t sec_size = WorldConfig::sector_size; 
    if (sec_size == 0) sec_size = 480; 
    sector = static_cast<uint8_t>(world_val / sec_size);
    uint32_t remainder = world_val % sec_size;
    rel = static_cast<uint8_t>((remainder * 256) / sec_size);
}

std::ostream& operator<<(std::ostream& out, const packet_set_food_abs& p) {
  out << static_cast<PacketBase>(p);
  for (const Food& f : *p.food_ptr) {
    out << write_uint8(f.color) << write_uint16(f.x) << write_uint16(f.y) << write_uint8(f.size * 5);
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const packet_set_food_rel& p) {
  out << static_cast<PacketBase>(p);
  if (p.food_ptr->empty()) return out;
  uint8_t sx, sy, rx, ry;
  get_sector_coords(p.food_ptr->at(0).x, sx, rx);
  get_sector_coords(p.food_ptr->at(0).y, sy, ry);
  out << write_uint8(sx) << write_uint8(sy);
  for (const Food& f : *p.food_ptr) {
    get_sector_coords(f.x, sx, rx);
    get_sector_coords(f.y, sy, ry);
    out << write_uint8(f.color) << write_uint8(rx) << write_uint8(ry) << write_uint8(f.size * 5);
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const packet_spawn_food& p) {
  out << static_cast<PacketBase>(p);
  if (p.is_modern) {
      uint8_t sx, sy, rx, ry;
      get_sector_coords(p.m_food.x, sx, rx);
      get_sector_coords(p.m_food.y, sy, ry);
      out << write_uint8(sx) << write_uint8(sy) << write_uint8(rx) << write_uint8(ry)
          << write_uint8(p.m_food.color) << write_uint8(p.m_food.size * 5);
  } else {
      out << write_uint8(p.m_food.color) << write_uint16(p.m_food.x) << write_uint16(p.m_food.y)
          << write_uint8(p.m_food.size * 5);
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const packet_add_food& p) {
  out << static_cast<PacketBase>(p);
  if (p.is_modern) {
      uint8_t sx, sy, rx, ry;
      get_sector_coords(p.m_food.x, sx, rx);
      get_sector_coords(p.m_food.y, sy, ry);
      out << write_uint8(sx) << write_uint8(sy) << write_uint8(rx) << write_uint8(ry)
          << write_uint8(p.m_food.color) << write_uint8(p.m_food.size * 5);
  } else {
      out << write_uint8(p.m_food.color) << write_uint16(p.m_food.x) << write_uint16(p.m_food.y)
          << write_uint8(p.m_food.size * 5);
  }
  return out;
}

// FIX: Handle 'C' for Modern, 'c' for Legacy
std::ostream& operator<<(std::ostream& out, const packet_eat_food& p) {
  out << static_cast<PacketBase>(p);

  // C Client (v31) -> Relative Coordinates
  if (p.protocol_version >= 20) {
      uint8_t sx, sy, rx, ry;
      get_sector_coords(p.m_food.x, sx, rx);
      get_sector_coords(p.m_food.y, sy, ry);

      out << write_uint8(sx) << write_uint8(sy)
          << write_uint8(rx) << write_uint8(ry);
  } 
  // Legacy/JS Client (v1-14) -> Absolute Coordinates
  else {
      out << write_uint16(p.m_food.x)
          << write_uint16(p.m_food.y);
  }

  // Eater ID (Same for both)
  if (p.snakeId > 0) {
    out << write_uint16(p.snakeId);
  }
  
  return out;
}