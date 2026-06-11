/*
==================================================
FILE: p_minimap.cc
RELATIVE PATH: packet/p_minimap.cc
==================================================
*/
#include "packet/p_minimap.h"

std::ostream& operator<<(std::ostream& out, const packet_minimap& p) {
  out << static_cast<PacketBase>(p);
  
  // Only write size header for 'M' packets (C Client)
  // Legacy 'u' packets (JS Client) assume 80x80 and have no header
  if (p.packet_type == packet_t_minimap) {
      out << write_uint16(p.size);
  }
  
  for (const uint8_t v : p.data) {
    out.put(v);
  }
  return out;
}