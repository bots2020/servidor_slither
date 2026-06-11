/*
==================================================
FILE: config.h
RELATIVE PATH: game/config.h
==================================================
*/
#ifndef SRC_GAME_CONFIG_H_
#define SRC_GAME_CONFIG_H_

#include <cstdint>

typedef uint16_t snake_id_t; // Tipo base para identificar uma cobra por id (compactado em 16 bits).

struct WorldConfig {
  // Quantidade de bots gerados no início do servidor (0 = nenhum bot).
  uint16_t bots = 0; // Número de bots para spawnar no start (configurável).

  // Se deve respawnar bots quando eles morrem (true = respawn automático).
  bool bot_respawn = true; // Controle do respawn de bots.

  // Número de itens de comida que devem nascer por tick do mundo.
  uint16_t food_spawn_rate = 0; // Taxa de spawn de comida.

  // --- NOVO: Pesos de escolha de setor para spawn de comida ---
  // Pesos funcionam como proporções. Ex.: 25, 25, 50 => 25% perto, 25% no setor da cobra, 50% aleatório.
  uint16_t spawn_prob_near_snake = 25; // Peso para escolher um setor vizinho de uma cobra.
  uint16_t spawn_prob_on_snake = 25;   // Peso para escolher o setor onde uma cobra está.
  uint16_t spawn_prob_random = 50;     // Peso para escolher um setor totalmente aleatório.
  // -------------------------------------------

  // Configurações de score/length para começar uma cobra humana e uma bot.
  uint16_t h_snake_start_score = 5; // Score inicial da cobra humana (antes de converter para tamanho).
  uint16_t b_snake_start_score = 5; // Score inicial da cobra bot.

  // Tamanho mínimo inicial (em número de partes) para permitir animação de crescimento.
  uint16_t snake_min_length = 2; // Comprimento mínimo inicial (em partes).

  // Configurações do boost (custo e tamanho do drop).
  uint16_t boost_cost = 20; // Custo de fullness (ou recurso equivalente) para ativar boost.
  uint8_t boost_drop_size = 10; // Tamanho/quantidade do “drop” gerado ao usar boost.

  // Valores originais (referência do protocolo/base do Slither.io).
  static const uint16_t game_radius = 3584; // Raio do mapa (limite circular do mundo) — ajustado a partir do PacketInit do servidor privado.
  static const uint16_t max_snake_parts = 430; // Limite máximo de partes por cobra — ajustado a partir do PacketInit do servidor privado.

  static const uint16_t sector_size = 256; // Tamanho (diâmetro lógico) de cada setor em coordenadas do mundo — ajustado a partir do PacketInit.
  static const uint16_t sector_count_along_edge = 28; // Quantidade de setores ao longo do lado/gradiente do grid — ajustado a partir do PacketInit.

  // Distância radial onde a cobra “morre” (usado para detectar colisão com borda).
  static const uint16_t death_radius = game_radius; // Raio de morte alinhado ao limite real do mapa (evita “morrer no meio do nada” quando sector_size muda).

  // sqrt(2) * sector_size. Para sector_size=256 => ~362.
  static const uint16_t sector_diag_size = 362; // Diagonal do setor usada em interseção/escala de colisão.
  static const uint16_t move_step_distance = 42; // Distância percorrida por passo de movimento em ticks.

  // Tempo base do frame (tick do servidor) em milissegundos.
  static const long frame_time_ms = 8; // Intervalo do loop em ms (mundo/frames).


  
  // Versão do protocolo suportada neste servidor.
  static const uint8_t protocol_version = 14; // Versão do protocolo do jogo — ajustado a partir do PacketInit.
};

#endif  // SRC_GAME_CONFIG_H_
