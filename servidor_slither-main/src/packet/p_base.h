/*
==================================================
FILE: p_base.h
RELATIVE PATH: packet/p_base.h
==================================================
*/
#ifndef SRC_PACKET_P_BASE_H_
#define SRC_PACKET_P_BASE_H_

#include <iostream>
#include "packet/p_format.h"

enum out_packet_t : uint8_t;

struct PacketBase {
  uint16_t client_time = 0;  // 2 bytes - time since last message from client
  out_packet_t packet_type;  // 1 byte - packet type

  PacketBase() = default;
  explicit PacketBase(out_packet_t t) : packet_type(t) {}
  PacketBase(out_packet_t t, uint16_t clock) : client_time(clock), packet_type(t) {}

  size_t get_size() const noexcept { return 3; }
};

enum in_packet_t : uint8_t {
  in_packet_t_start_login = 'c',
  in_packet_t_verify_code = 'o',
  in_packet_t_username_skin = 's',
  in_packet_t_rotation = 252,
  in_packet_t_angle = 0,
  in_packet_t_ping = 251,
  in_packet_t_rot_left = 108,
  in_packet_t_rot_right = 114,
  in_packet_t_start_acc = 253,
  in_packet_t_stop_acc = 254,
  in_packet_t_victory_message = 255,
};

enum out_packet_t : uint8_t {
  packet_t_init = 'a',
  packet_t_rot_ccw_wang_sp = 'E',
  packet_t_rot_ccw_wang = 'E',
  packet_t_rot_ccw_ang_wang = '3',
  packet_t_rot_ccw_sp = '3',
  packet_t_rot_ccw_ang_wang_sp = 'e',
  packet_t_rot_ccw_ang_sp = 'e',
  packet_t_rot_ccw_ang = 'e',
  packet_t_rot_cw_ang_wang_sp = '4',
  packet_t_rot_cw_wang_sp = '4',
  packet_t_rot_cw_wang = '4',
  packet_t_rot_cw_ang_wang = '5',
  packet_t_set_fullness = 'h',
  packet_t_rem_part = 'r',
  packet_t_mov = 'g',
  packet_t_mov_rel = 'G',
  packet_t_inc = 'n',
  packet_t_inc_rel = 'N',
  packet_t_leaderboard = 'l',
  packet_t_end = 'v',
  packet_t_add_sector = 'W',
  packet_t_rem_sector = 'w',
  packet_t_highscore = 'm',
  packet_t_pong = 'p',
  
  // MINIMAP TYPES
  packet_t_minimap = 'M',        // Modern/C Client (Reverse encoded + Size Header)
  packet_t_minimap_legacy = 'u', // JS Client (Forward encoded + No Header)

  packet_t_snake = 's',
  packet_t_set_food = 'F',
  packet_t_spawn_food = 'b',
  packet_t_add_food = 'f',
  packet_t_eat_food = 'c',
  packet_t_eat_food_rel = '<',
  packet_t_mov_prey = 'j',
  packet_t_add_prey = 'y',
  packet_t_rem_prey = 'y',
  packet_t_kill = 'k',
  packet_d_reset = '0',
  packet_d_draw = '!',
};

std::ostream& operator<<(std::ostream& out, const PacketBase& p);
std::istream& operator>>(std::istream& in, in_packet_t& p);

#endif  // SRC_PACKET_P_BASE_H_