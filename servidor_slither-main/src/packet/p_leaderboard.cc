#include "packet/p_leaderboard.h"

std::ostream& operator<<(std::ostream& out, const packet_leaderboard& p) {
  out << static_cast<PacketBase>(p);
  // Byte 3: Local player rank (uint8) - strictly 0-255 or 0 if not on board
  out << write_uint8(p.leaderboard_rank);
  // Byte 4-5: Local rank (uint16)
  out << write_uint16(p.local_rank);
  // Byte 6-7: Player count
  out << write_uint16(p.players);
  
  for (const auto& ptr : p.top) {
    out << write_uint16(ptr->parts.size());
    out << write_fp24(ptr->fullness / 100.0f);
    out << write_uint8(ptr->skin); // Font color in docs, often mapped to skin or constant
    out << write_string(ptr->name);
  }
  return out;
}
