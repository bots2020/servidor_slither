#ifndef SRC_PACKET_P_INIT_H_
#define SRC_PACKET_P_INIT_H_

#include "packet/p_base.h"

struct PacketInit : public PacketBase {
  PacketInit() : PacketBase(packet_t_init) {} // Constrói o pacote 'init' inicializando a base com o type 'packet_t_init'.

  uint32_t game_radius = 3584; // Raio do mapa (codificado no protocolo como int24).
  uint16_t max_snake_parts = 430; // Limite máximo de partes por cobra (int16).
  uint16_t sector_size = 256; // Tamanho de cada setor no grid (int16).
  uint16_t sector_count_along_edge = 28; // Quantidade de setores ao longo do lado do grid (int16).
  float spangdv = 4.8f; // Parâmetro spangdv do protocolo (armazenado como int8 no encoding).
  float nsp1 = 5.39f; // Parâmetro nsp1 do protocolo (int16 no encoding).
  float nsp2 = 0.4f;  // Parâmetro nsp2 do protocolo (int16 no encoding).
  float nsp3 = 14.0f; // Parâmetro nsp3 do protocolo (int16 no encoding).
  float snake_ang_speed = 0.033f; // Velocidade angular da cobra (int16 no encoding).
  float prey_ang_speed = 0.029f; // Velocidade angular da presa/prey (int16 no encoding).
  float snake_tail_k = 0.43f; // Constante do tail da cobra (int16 no encoding).
  uint8_t protocol_version = 13; // Versão do protocolo suportada (int8).
  
  // FIX: Add padding to match 32-byte packet size from original log
  // Original: Type: a, Len: 32
  // Current: 26 bytes + 6 bytes padding = 32 bytes
  uint8_t padding[6] = {0, 0, 0, 0, 0, 0}; // Preenchimento para garantir tamanho total do pacote em 32 bytes.

  size_t get_size() const noexcept { return 32; } // Tamanho total fixo do pacote init (32 bytes).
};

std::ostream& operator<<(std::ostream& out, const PacketInit& p);

#endif  // SRC_PACKET_P_INIT_H_
