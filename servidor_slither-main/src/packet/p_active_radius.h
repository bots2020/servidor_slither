#ifndef SRC_PACKET_P_ACTIVE_RADIUS_H_
#define SRC_PACKET_P_ACTIVE_RADIUS_H_

#include "packet/p_base.h"

struct packet_active_radius : public PacketBase {
  packet_active_radius() : PacketBase(packet_t_active_radius) {}
  uint32_t value = 0;
  size_t get_size() const noexcept { return 6; }
};

inline std::ostream& operator<<(std::ostream& out, const packet_active_radius& p) {
  return out << static_cast<PacketBase>(p) << write_uint24(p.value);
}

#endif
