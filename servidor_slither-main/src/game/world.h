/*
==================================================
FILE: world.h
RELATIVE PATH: game/world.h
==================================================
*/
#ifndef SRC_GAME_WORLD_H_
#define SRC_GAME_WORLD_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

#include "game/sector.h"
#include "game/snake.h"

class World {
 public:
  void Init(WorldConfig in_config); // Inicializa o mundo com a configuração informada.
  void InitSectors(); // Cria e inicializa a estrutura de setores (grid espacial).
  void InitFood(); // Popula o mundo com comida inicial.

  void Tick(long dt); // Executa um “passo” do mundo (atualiza movimento, colisões etc.).

  Snake::Ptr CreateSnake(int start_len = 0); // Cria uma cobra no mundo (com comprimento inicial opcional).
  Snake::Ptr CreateSnakeBot(); // Cria uma cobra bot (com nome/skin padrão e marca bot).
  void SpawnNumSnakes(const int count); // Cria várias cobras (bots) no mundo.
  void CheckSnakeBounds(Snake *s); // Checa colisões/bordas para a cobra e marca estado de morte.

  void RegenerateFood(); // Regenera/realoca comida ao longo do tempo.

  void InitRandom(); // Inicializa o gerador de números aleatórios.
  int NextRandom(); // Retorna um próximo número aleatório inteiro.
  float NextRandomf(); // Retorna um próximo número aleatório float em [0,1] (por implementação).
  template <typename T>
  T NextRandom(T base); // Retorna um aleatório T limitado por “base”.

  void AddSnake(Snake::Ptr ptr); // Adiciona uma cobra ao mapa interno.
  void RemoveSnake(snake_id_t id); // Remove uma cobra do mundo e faz flush de mudanças associadas.
  SnakeMapIter GetSnake(snake_id_t id); // Obtém iterador da cobra pelo id (ou end se não existir).
  SnakeMap& GetSnakes(); // Retorna referência ao mapa de cobras.
  SectorSeq& GetSectors(); // Retorna referência à coleção de setores do grid.
  Ids& GetDead(); // Retorna referência à lista/mapa de cobras “mortas” aguardando remoção.

  SnakeVec& GetChangedSnakes(); // Retorna referência à lista de cobras que mudaram e precisam ser processadas.

  void FlushChanges(snake_id_t id); // Remove da lista de mudanças a cobra com o id fornecido.
  void FlushChanges(); // Limpa a lista de cobras “mudaram”.
 private:
  void TickSnakes(long dt); // Atualiza todas as cobras para o delta de tempo informado.
  
  // --- NEW: Helper to check for collisions before spawning ---
  bool IsLocationSafe(float x, float y, float safety_radius); // Verifica se uma posição inicial não colide com outras cobras.

 private:
  SnakeMap snakes; // Mapa de cobras ativas (id -> ponteiro).
  Ids dead; // IDs das cobras mortas/pendentes.
  SectorSeq sectors; // Grid de setores para acelerar colisões/visibilidade.
  SnakeVec changes; // Lista de cobras que tiveram alterações relevantes para replicação.

  uint16_t lastSnakeId = 0; // Último id de cobra gerado (incremental).
  long ticks = 0; // Acumulador de tempo do mundo (para frame_time_ms).
  uint32_t frames = 0; // Contador de frames (útil para debug/telemetria).

  WorldConfig config; // Configuração atual do mundo em uso.
};

std::ostream& operator<<(std::ostream& out, const World& w); // Serializa informações do mundo para logs/console.

#endif  // SRC_GAME_WORLD_H_
