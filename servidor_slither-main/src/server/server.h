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

typedef websocketpp::connection_hdl connection_hdl;
typedef websocketpp::frame::opcode::value opcode;
typedef websocketpp::lib::error_code error_code;

class WSPPServer : public websocketpp::server<WSPPServerConfig> {
 public:
  template <typename T>
  void send(connection_hdl hdl, T packet, opcode op, error_code &ec) {
    const connection_ptr con = get_con_from_hdl(hdl, ec);
    if (ec) {
      std::cerr << "[NET ERROR] Invalid connection handle" << std::endl;
      return;
    }

    // --- NUCLEAR FIX FOR SEGMENTATION FAULT ---
    // Instead of trying to guess the size and using a fixed array,
    // we ALWAYS use a dynamic, expandable buffer.
    // This prevents writing past the end of memory (Buffer Overflow).
    
    boost::asio::streambuf buf;
    std::ostream out(&buf);
    
    // Write packet. If it's larger than expected, 'buf' simply expands. No crash.
    out << packet;

    // Send the exact amount of data written
    ec = con->send(boost::asio::buffer_cast<void const *>(buf.data()), buf.size(), op);
  }

  template <typename T>
  void send_binary(connection_hdl hdl, T packet, error_code &ec) {
    send(hdl, packet, opcode::binary, ec);
  }

  template <typename T>
  void send_binary(connection_hdl hdl, T packet) {
    error_code ec;
    send_binary(hdl, packet, ec);
    if (ec) {
      std::cerr << "[NET ERROR] Send failed: " << ec.message() << std::endl;
    }
  }
};

typedef WSPPServer::message_ptr message_ptr;

#endif  // SRC_SERVER_SERVER_H_