#include "packet/p_init.h"

std::ostream& operator<<(std::ostream& out, const PacketInit& p) { // Serializa o pacote 'PacketInit' no stream.
  return out << static_cast<PacketBase>(p) // Escreve o cabeçalho base do pacote (inclui o type).
             << write_uint24(p.game_radius) // Escreve o raio do mapa (game_radius) com 24 bits.
             << write_uint16(p.max_snake_parts) // Escreve o tamanho máximo da cobra (max_snake_parts).
             << write_uint16(p.sector_size) // Escreve o tamanho do setor (sector_size).
             << write_uint16(p.sector_count_along_edge) // Escreve a quantidade de setores no eixo (grid).
             << write_fp8(p.spangdv) // Escreve spangdv no formato fp8 do protocolo.
             << write_fp16<2>(p.nsp1) // Escreve nsp1 no formato fp16<2> do protocolo.
             << write_fp16<2>(p.nsp2) // Escreve nsp2 no formato fp16<2> do protocolo.
             << write_fp16<2>(p.nsp3) // Escreve nsp3 no formato fp16<2> do protocolo.
             << write_fp16<3>(p.snake_ang_speed) // Escreve snake_ang_speed no formato fp16<3>.
             << write_fp16<3>(p.prey_ang_speed) // Escreve prey_ang_speed no formato fp16<3>.
             << write_fp16<3>(p.snake_tail_k) // Escreve snake_tail_k no formato fp16<3>.
             << write_uint8(p.protocol_version) // Escreve a versão do protocolo (protocol_version).
             // FIX: Match the "Good Server" tail bytes exactly // Garante que os bytes finais batam com o servidor “bom”.
             // Byte 26: 42 (0x2A) - Likely move_step_distance // Byte 26 (42) - provavelmente move_step_distance.
             // Bytes 27-31: From working server log // Bytes 27-31 (finais) - copiados do log do servidor funcional.
             << write_uint8(42) // 0x2A // Escreve 42 como byte 26.
             << write_uint8(0) << write_uint8(0) << write_uint8(0) // Preenche bytes intermediários com zero.
             << write_uint8(13)  // 0x0D // Tail byte alinhado ao servidor privado que desenha a parede/anel corretamente.
             << write_uint8(184); // 0xB8 // Tail byte final alinhado ao servidor privado.
}
