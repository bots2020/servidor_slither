#include "packet/p_inc.h"

std::ostream& operator<<(std::ostream& out, const packet_inc& p) { // Serializa o pacote 'packet_inc' no stream de saída.
  out << static_cast<PacketBase>(p); // Escreve primeiro o cabeçalho/base do pacote (inclui type).
  out << write_uint16(p.snakeId); // Escreve o id da cobra (16 bits).
  out << write_uint16(p.x); // Escreve a coordenada X da cabeça (16 bits).
  out << write_uint16(p.y); // Escreve a coordenada Y da cabeça (16 bits).
  out << write_fp24(p.fullness / 100.0f); // Escreve a fullness como fp24 normalizada (dividida por 100).
  return out; // Retorna o stream para permitir encadeamento.
}

std::ostream& operator<<(std::ostream& out, const packet_inc_rel& p) { // Serializa o pacote 'packet_inc_rel' (movimento relativo) no stream.
  out << static_cast<PacketBase>(p); // Escreve primeiro a base do pacote (inclui type).
  out << write_uint16(p.snakeId); // Escreve o id da cobra (16 bits).
  out << write_uint8(p.dx); // Escreve o delta X relativo (8 bits).
  out << write_uint8(p.dy); // Escreve o delta Y relativo (8 bits).
  out << write_fp24(p.fullness / 100.0f); // Escreve a fullness como fp24 normalizada (dividida por 100).
  return out; // Retorna o stream para permitir encadeamento.
}
