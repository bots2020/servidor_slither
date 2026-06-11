/*
==================================================
FILE: p_snake.cc
RELATIVE PATH: packet/p_snake.cc
==================================================
*/

#include "packet/p_snake.h"

// Implementation for ADDING a snake (Spawning)
std::ostream& operator<<(std::ostream& out, const packet_add_snake& p) {
  out << static_cast<PacketBase>(p);

  const Snake* s = p.s;

  // 1. Header: [ID][Angle][Reserved][WantedAngle][Speed][Fullness][Skin][X][Y]
  out << write_uint16(s->id);
  out << write_ang24(s->angle); 
  out << write_uint8(0);        // Reserved byte (must be 0)
  out << write_ang24(s->wangle); 
  out << write_uint16(static_cast<uint16_t>(s->speed * 1000.0f / 32.0f)); 
  out << write_fp24(s->fullness / 100.0f);
  out << write_uint8(s->skin);
  out << write_uint24(s->get_head_x() * 5.0f);
  out << write_uint24(s->get_head_y() * 5.0f);
  
  // 2. Name: [Len][String]
  out << write_string(s->name); 

  // 3. Custom Skin Data: [Len][Data]
  if (s->custom_skin_data.empty()) {
      out << write_uint8(0);
  } else {
      // Safety clamp to prevent packet corruption
      if (s->custom_skin_data.size() > 255) {
          out << write_uint8(255); 
          out.write(s->custom_skin_data.data(), 255);
      } else {
          out << write_string(s->custom_skin_data);
      }
  }

  // 4. Accessory/Padding Byte
  // Most clients expect a padding byte between skin data and body parts
  out << write_uint8(0);

  // 5. Body Parts
  if (!s->parts.empty()) {
    // Protocol sends the TAIL position in absolute coords first
    const Body& tail = s->parts.back();
    float tx = tail.x;
    float ty = tail.y;
    
    out << write_uint24(static_cast<uint24_t>(tx * 5.0f))
        << write_uint24(static_cast<uint24_t>(ty * 5.0f));

    // Then relative coords from Tail -> Head
    for (size_t i = s->parts.size() - 1; i > 0; --i) {
        const Body& curr = s->parts[i];
        const Body& next = s->parts[i-1]; // Moving towards head

        float dx = next.x - curr.x;
        float dy = next.y - curr.y;

        out << write_uint8(static_cast<uint8_t>(dx * 2.0f + 127.0f))
            << write_uint8(static_cast<uint8_t>(dy * 2.0f + 127.0f));
    }
  }

  return out;
}

// Implementation for REMOVING a snake (Death/Despawn)
std::ostream& operator<<(std::ostream& out, const packet_remove_snake& p) {
  out << static_cast<PacketBase>(p);
  out << write_uint16(p.snakeId);
  out << write_uint8(p.status); // 0 = Left range, 1 = Died
  return out;
}