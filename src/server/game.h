/*
==================================================
FILE: game.h
RELATIVE PATH: server/game.h
==================================================
*/
#ifndef SRC_SERVER_GAME_H_
#define SRC_SERVER_GAME_H_

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <mutex>

#include "server/server.h"
#include "game/world.h"
#include "packet/d_all.h"
#include "packet/p_all.h"

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BOLD    "\033[1m"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

struct Session {
  snake_id_t snake_id = 0; // Id da cobra associada à sessão (0 = ainda não spawnou).
  long last_packet_time = 0; // Timestamp do último pacote recebido/enviado.

  long death_timestamp = 0; // Timestamp em que a cobra morreu (0 = vivo).

  std::string name; // Nome do jogador/cobra (para exibição).
  std::string message; // Mensagem/vitória (se suportado pelo protocolo).
  std::string custom_skin_data; // Dados customizados de skin (skin custom/c).

  uint8_t protocol_version = 0; // Versão do protocolo do cliente conectado.
  uint8_t skin = 0; // Id da skin usada pelo cliente.

  bool is_modern_protocol() const {  // Retorna se o cliente é “moderno” (usa packing/format diferenciado).
      return protocol_version >= 25; 
  }

  Session() = default; // Construtor padrão (sessão “vazia”).
  Session(snake_id_t id, long now) : snake_id(id), last_packet_time(now) {} // Constrói sessão já com id e timestamp.
};

class GameServer {
 public:
  GameServer(); // Construtor do servidor (registra handlers de websocket).
  int Run(IncomingConfig in_config); // Inicializa e executa o loop do servidor.
  PacketInit BuildInitPacket(); // Monta o pacote 'init' para envio aos clientes.

  typedef std::unordered_map<snake_id_t, connection_hdl> ConnectionMap; // Mapeia id da cobra -> handle da conexão.
  typedef std::map<connection_hdl, Session, std::owner_less<connection_hdl>> SessionMap; // Mapeia handle -> Session.
  typedef SessionMap::iterator SessionIter; // Iterator para percorrer sessões.

 private:
  void on_socket_init(connection_hdl, boost::asio::ip::tcp::socket &s); // Handler inicial do socket (ex.: TCP no_delay).
  void on_open(connection_hdl hdl); // Handler chamado quando conexão abre.
  void on_message(connection_hdl hdl, message_ptr ptr); // Handler chamado quando chega mensagem binária/texto.
  void on_close(connection_hdl hdl); // Handler chamado quando conexão fecha.
  void on_timer(error_code const &ec); // Handler chamado periodicamente pelo timer do websocketpp.

  void SendPOVUpdateTo(SessionIter ses_i, Snake *ptr);
  void SendFoodUpdate(Snake *ptr);
  void BroadcastDebug();
  void BroadcastUpdates();
  void BroadcastLeaderboard();
  void BroadcastMinimap();
  
  void CleanupDeadSessions();
  void SpawnBot();

  long last_leaderboard_time = 0; // Timestamp da última transmissão de leaderboard.
  long last_minimap_time = 0; // Timestamp da última transmissão de minimapa.

  SessionIter LoadSessionIter(snake_id_t id);
  void DoSnake(snake_id_t id, std::function<void(Snake *)> f);
  void RemoveSnake(snake_id_t id);
  void RemoveDeadSnakes();
  long GetCurrentTime();
  void NextTick(long last);
  void PrintWorldInfo();

 private:
  // ... (templates and private members remain the same)
  static std::string packet_to_hex(const std::string& data, size_t max_bytes = 32) { // Converte bytes do pacote em string hex para debug.
    std::stringstream ss;
    size_t len = std::min(data.size(), max_bytes);
    for (size_t i = 0; i < len; ++i) {
      ss << std::hex << std::setfill('0') << std::setw(2) 
         << (int)(unsigned char)data[i] << " ";
    }
    if (data.size() > max_bytes) ss << "...";
    return ss.str();
  }

  template <typename T>
  void send_binary(SessionMap::iterator s, T packet) {
    const long now = GetCurrentTime();
    const uint16_t interval = static_cast<uint16_t>(now - s->second.last_packet_time);
    s->second.last_packet_time = now;
    packet.client_time = interval;
    endpoint.send_binary(s->first, packet);
  }

  template <typename T>
  void broadcast_binary(T packet) {
    const long now = GetCurrentTime();
    for (auto &s : sessions) {
      if (s.second.snake_id == 0) continue;
      const uint16_t interval = static_cast<uint16_t>(now - s.second.last_packet_time);
      s.second.last_packet_time = now;
      packet.client_time = interval;
      endpoint.send_binary(s.first, packet);
    }
  }

  template <typename T>
  void broadcast_debug(T packet) {
    for (auto &s : sessions) {
      endpoint.send_binary(s.first, packet);
    }
  }

  WSPPServer endpoint; // Endpoint websocketpp que roda o server.
  WSPPServer::timer_ptr timer; // Ponteiro/handle do timer usado no on_timer.
  long last_time_point; // Último timestamp usado para calcular dt do mundo.
  static const long timer_interval_ms = 10; // Intervalo fixo do timer (ms).

  World world; // Estado atual do mundo/jogo (cobras, comida, setores).
  PacketInit init; // Pacote init pré-montado enviado aos clientes no login.
  IncomingConfig config; // Configuração do servidor (parseada da linha de comando).
  SessionMap sessions; // Mapa de sessões (conexão -> dados do cliente).
  ConnectionMap connections; // Mapa auxiliar (id da cobra -> conexão).
  
  std::mutex game_mutex;
};

#endif  // SRC_SERVER_GAME_H_
