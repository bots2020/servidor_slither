#ifndef SRC_PACKET_P_MINIMAP_H_
#define SRC_PACKET_P_MINIMAP_H_

#include "packet/p_base.h"
#include <vector>

struct packet_minimap : public PacketBase {
  packet_minimap(uint16_t sz) : PacketBase(packet_t_minimap), size(sz) {}

  uint16_t size; // Grid dimension (e.g., 80)
  std::vector<uint8_t> data;

  // Header (3) + Size (2) + Data
  size_t get_size() const noexcept { return 3 + 2 + data.size(); }
};

std::ostream& operator<<(std::ostream& out, const packet_minimap& p);

#endif  // SRC_PACKET_P_MINIMAP_H_