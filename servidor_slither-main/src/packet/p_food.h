#ifndef SRC_PACKET_P_FOOD_H_
#define SRC_PACKET_P_FOOD_H_

#include <vector>

#include "game/food.h"
#include "packet/p_base.h"

// ==========================================================
// 1. Initial Sector Food (Packet 'F')
// ==========================================================

// Legacy: Absolute Coordinates (Protocol < 20)
struct packet_set_food_abs : public PacketBase {
  explicit packet_set_food_abs(const std::vector<Food>* ptr)
      : PacketBase(packet_t_set_food), food_ptr(ptr) {}

  const std::vector<Food>* food_ptr;
  // Size estimate: Header + (6 bytes per food)
  size_t get_size() const noexcept { return 3 + food_ptr->size() * 6; }
};

// Modern: Relative/Sector Coordinates (Protocol >= 20)
struct packet_set_food_rel : public PacketBase {
  explicit packet_set_food_rel(const std::vector<Food>* ptr)
      : PacketBase(packet_t_set_food), food_ptr(ptr) {}

  const std::vector<Food>* food_ptr;
  // Size estimate: Header + (6 bytes per food)
  size_t get_size() const noexcept { return 3 + food_ptr->size() * 6; }
};


// ==========================================================
// 2. Spawn Food (Packet 'b' - Turbo/Death)
// ==========================================================
struct packet_spawn_food : public PacketBase {
  packet_spawn_food() : PacketBase(packet_t_spawn_food) {}
  
  // Added is_modern flag to constructor
  packet_spawn_food(Food f, bool modern)
      : PacketBase(packet_t_spawn_food), m_food(f), is_modern(modern) {}

  Food m_food;
  bool is_modern = false;

  size_t get_size() const noexcept { return 3 + 6; }
};


// ==========================================================
// 3. Add Natural Food (Packet 'f')
// ==========================================================
struct packet_add_food : public PacketBase {
  packet_add_food() : PacketBase(packet_t_add_food) {}
  
  // Added is_modern flag to constructor
  packet_add_food(Food f, bool modern)
      : PacketBase(packet_t_add_food), m_food(f), is_modern(modern) {}

  Food m_food;
  bool is_modern = false;

  size_t get_size() const noexcept { return 3 + 6; }
};


// ==========================================================
// 4. Eat Food (Packet 'c')
// ==========================================================
struct packet_eat_food : public PacketBase {
  // Update constructor: 
  // If version >= 20 (C Client is v31), use '<' (rel).
  // If version < 20 (JS Client is v14), use 'c' (abs).
  packet_eat_food(uint16_t id, Food f, uint8_t ver)
      : PacketBase(ver >= 20 ? packet_t_eat_food_rel : packet_t_eat_food), 
        m_food(f), snakeId(id), protocol_version(ver) {}

  Food m_food;
  uint16_t snakeId = 0;
  uint8_t protocol_version = 0;

  size_t get_size() const noexcept { return 3 + 6; }
};

// Stream operators
std::ostream& operator<<(std::ostream& out, const packet_set_food_abs& p);
std::ostream& operator<<(std::ostream& out, const packet_set_food_rel& p);
std::ostream& operator<<(std::ostream& out, const packet_spawn_food& p);
std::ostream& operator<<(std::ostream& out, const packet_add_food& p);
std::ostream& operator<<(std::ostream& out, const packet_eat_food& p);

#endif  // SRC_PACKET_P_FOOD_H_