/*
==================================================
FILE: server.h
RELATIVE PATH: server/server.h
==================================================
*/

#ifndef SRC_SERVER_SERVER_H_
#define SRC_SERVER_SERVER_H_

#include <iostream>
#include <websocketpp/server.hpp>

#include "server/config.h"
// #include "server/streambuf_array.h" // DISABLED: Causing Stack Overflows

typedef websocketpp::connection_hdl connection_hdl; // Tipo alias do websocketpp para handle de conexão.
typedef websocketpp::frame::opcode::value opcode; // Tipo alias para opcode do frame.
typedef websocketpp::lib::error_code error_code; // Tipo alias para erros do websocketpp/lib asio.

class WSPPServer : public websocketpp::server<WSPPServerConfig> { // Wrapper do servidor websocketpp com helpers de envio.
 public:
  template <typename T>
  void send(connection_hdl hdl, T packet, opcode op, error_code &ec) { // Envia pacote (serializado em stream) para um cliente.
    const connection_ptr con = get_con_from_hdl(hdl, ec); // Resolve handle -> conexão.
    if (ec) { // Se falhar ao resolver conexão.
      std::cerr << "[NET ERROR] Invalid connection handle" << std::endl; // Log do erro.
      return; // Não tenta enviar.
    }

    // --- NUCLEAR FIX FOR SEGMENTATION FAULT ---
    // Instead of trying to guess the size and using a fixed array,
    // we ALWAYS use a dynamic, expandable buffer.
    // This prevents writing past the end of memory (Buffer Overflow).
    
    boost::asio::streambuf buf; // Buffer dinâmico para escrita do pacote.
    std::ostream out(&buf); // Stream que escreve no streambuf.

    // Escreve o pacote no stream; se crescer, o buffer expande sem crash.
    out << packet; // Serializa 'packet' via operator<<.

    // Envia exatamente a quantidade de bytes escritos no buffer.
    ec = con->send(boost::asio::buffer_cast<void const *>(buf.data()), buf.size(), op); // Envio ao socket.
  }

  template <typename T>
  void send_binary(connection_hdl hdl, T packet, error_code &ec) { // Envia pacote como binário com ec explícito.
    send(hdl, packet, opcode::binary, ec); // Delegação para send().
  }

  template <typename T>
  void send_binary(connection_hdl hdl, T packet) { // Envia pacote binário e loga erro automaticamente.
    error_code ec; // Variável de erro.
    send_binary(hdl, packet, ec); // Envia capturando ec.
    if (ec) { // Se falhar.
      std::cerr << "[NET ERROR] Send failed: " << ec.message() << std::endl; // Loga mensagem do erro.
    }
  }
};

typedef WSPPServer::message_ptr message_ptr;

#endif  // SRC_SERVER_SERVER_H_
