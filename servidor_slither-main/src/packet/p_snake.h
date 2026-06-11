/*
==================================================
FILE: p_snake.h
RELATIVE PATH: packet/p_snake.h
==================================================
*/
#ifndef SRC_PACKET_P_SNAKE_H_
#define SRC_PACKET_P_SNAKE_H_

#include "game/snake.h"
#include "packet/p_base.h"

struct packet_add_snake : public PacketBase {
  packet_add_snake() : PacketBase(packet_t_snake), s(nullptr) {}

  packet_add_snake(const Snake* input, bool modern)
      : PacketBase(packet_t_snake), s(input), is_modern(modern) {}

  const Snake* s;
  bool is_modern = false;

  size_t get_size() const noexcept {
    // 64-byte margin + body parts logic
    size_t body_size = s->parts.empty() ? 0 : (s->parts.size() - 1) * 2;
    return 33 + 64 + 
           s->name.length() + 
           s->custom_skin_data.length() + 
           body_size;
  }
};

struct packet_remove_snake : public PacketBase {
  packet_remove_snake() : PacketBase(packet_t_snake) {}
  packet_remove_snake(uint16_t id, uint8_t st)
      : PacketBase(packet_t_snake), snakeId(id), status(st) {}

  uint16_t snakeId = 0; 
  uint8_t status = status_snake_left; 

  size_t get_size() const noexcept { return 6; }

  static const uint8_t status_snake_left = 0;
  static const uint8_t status_snake_died = 1;
};

std::ostream& operator<<(std::ostream& out, const packet_add_snake& p);
std::ostream& operator<<(std::ostream& out, const packet_remove_snake& p);

#endif  // SRC_PACKET_P_SNAKE_H_