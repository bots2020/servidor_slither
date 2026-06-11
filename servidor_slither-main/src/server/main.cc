#include "server/game.h"

int main(const int argc, const char* const argv[]) { // Ponto de entrada do executável (cria servidor e roda).
  return std::unique_ptr<GameServer>(new GameServer())->Run(ParseCommandLine(argc, argv)); // Parse comando -> Run do servidor.
}
