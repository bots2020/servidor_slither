#ifndef SRC_SERVER_CONFIG_H_
#define SRC_SERVER_CONFIG_H_

#include "game/config.h"

#include <websocketpp/config/asio_no_tls.hpp>
// #include <websocketpp/extensions/permessage_deflate/enabled.hpp>

using websocketpp::log::alevel;
using websocketpp::log::elevel;

struct IncomingConfig {
  uint16_t port = 8080; // Porta TCP onde o servidor escuta.
  
  bool help = false; // Se true, imprime ajuda no startup.
  bool version = false; // Se true, imprime versão do servidor.
  bool verbose = false; // Se true, habilita logs verbosos.
  bool debug = false; // Se true, habilita modo debug (ex.: debug packets).

  WorldConfig world; // Configuração do mundo/jogo (tamanho, bots, food spawn etc.).
};

IncomingConfig ParseCommandLine(const int argc, const char *const *argv); // Lê argv/argc e retorna configurações parseadas via command line.

struct WSPPServerConfig : public websocketpp::config::asio {
  // Puxa configurações default da config base do websocketpp.
  typedef websocketpp::config::asio core; // Alias para “core” (config base).

  typedef core::concurrency_type concurrency_type; // Tipo usado pelo mecanismo de concorrência.
  typedef core::request_type request_type; // Tipo de request.
  typedef core::response_type response_type; // Tipo de response.
  typedef core::message_type message_type; // Tipo de mensagem.

  // TODO(john.koepi): fornecer pool managers para melhorar performance.
  typedef core::con_msg_manager_type con_msg_manager_type; // Gerenciador de mensagens por conexão.
  typedef core::endpoint_msg_manager_type endpoint_msg_manager_type; // Gerenciador de mensagens no endpoint.

  typedef core::alog_type alog_type; // Tipo do log de aplicativo.
  typedef core::elog_type elog_type; // Tipo do log de erro.
  typedef core::rng_type rng_type; // Tipo de gerador aleatório.
  typedef core::endpoint_base endpoint_base; // Tipo base do endpoint.

  static bool const enable_multithreading = true; // Habilita multithreading no websocketpp.

  struct transport_config : public core::transport_config {
    typedef core::concurrency_type concurrency_type; // Tipo de concorrência para o transport.
    typedef core::elog_type elog_type; // Tipo de log de erro no transport.
    typedef core::alog_type alog_type; // Tipo de log de app no transport.
    typedef core::request_type request_type; // Tipo de request no transport.
    typedef core::response_type response_type; // Tipo de response no transport.

    static bool const enable_multithreading = true; // Transport com multithreading habilitado.
  };

  typedef websocketpp::transport::asio::endpoint<transport_config> transport_type; // Endpoint de transport ASIO.

  /// static const websocketpp::log::level elog_level =
  ///    websocketpp::log::elevel::none;
  /// static const websocketpp::log::level alog_level =
  ///    websocketpp::log::alevel::none;

  /// permessage_compress extension
  // struct permessage_deflate_config {};

  // typedef websocketpp::extensions::permessage_deflate::enabled
  //    <permessage_deflate_config> permessage_deflate_type;
};

#endif  // SRC_SERVER_CONFIG_H_
