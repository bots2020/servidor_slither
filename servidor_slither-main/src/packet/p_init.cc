#include "packet/p_init.h"

std::ostream& operator<<(std::ostream& out, const PacketInit& p) {
  return out << static_cast<PacketBase>(p) << write_uint24(p.game_radius)
             << write_uint16(p.max_snake_parts) << write_uint16(p.sector_size)
             << write_uint16(p.sector_count_along_edge) << write_fp8(p.spangdv)
             << write_fp16<2>(p.nsp1) << write_fp16<2>(p.nsp2)
             << write_fp16<2>(p.nsp3) << write_fp16<3>(p.snake_ang_speed)
             << write_fp16<3>(p.prey_ang_speed) << write_fp16<3>(p.snake_tail_k)
             << write_uint8(p.protocol_version)
             // FIX: Match the "Good Server" tail bytes exactly
             // Byte 26: 42 (0x2A) - Likely move_step_distance
             // Bytes 27-31: From working server log
             << write_uint8(42)
             << write_uint8(0) << write_uint8(0)
             << write_uint8(0x00) << write_uint8(0x0D) << write_uint8(0xB8);
}
