#ifndef SRC_PACKET_P_INIT_H_
#define SRC_PACKET_P_INIT_H_

#include "packet/p_base.h"

struct PacketInit : public PacketBase {
  PacketInit() : PacketBase(packet_t_init) {}

  uint32_t game_radius = 21600;    // 3-5, int24
  uint16_t max_snake_parts = 411;  // 6-7, int16
  uint16_t sector_size = 300;      // 8-9, int16
  uint16_t sector_count_along_edge = 144;  // 10-11, int16
  float spangdv = 4.8f;  // 12, int8
  float nsp1 = 5.39f;  // 13-14, int16
  float nsp2 = 0.4f;   // 15-16, int16
  float nsp3 = 14.0f;  // 17-18, int16
  float snake_ang_speed = 0.033f;  // 19-20, int16
  float prey_ang_speed = 0.028f;  // 21-22, int16
  float snake_tail_k = 0.43f;     // 23-24, int16
  uint8_t protocol_version = 14;   // 25, int8
  
  // FIX: Add padding to match 32-byte packet size from original log
  // Original: Type: a, Len: 32
  // Current: 26 bytes + 6 bytes padding = 32 bytes
  uint8_t padding[6] = {0, 0, 0, 0, 0, 0};

  size_t get_size() const noexcept { return 32; }
};

std::ostream& operator<<(std::ostream& out, const PacketInit& p);

#endif  // SRC_PACKET_P_INIT_H_
