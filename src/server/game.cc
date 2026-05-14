#include "server/game.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "game/math.h"


// ANSI color codes for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"

// Helper function to convert packet data to hex string
static std::string to_hex(const std::string& data, size_t max_bytes = 64) { // Converte bytes do pacote em string hexadecimal (para debug).
  std::stringstream ss; // Stream que acumula a saída formatada.
  size_t len = std::min(data.size(), max_bytes); // Limita quantos bytes serão mostrados.
  for (size_t i = 0; i < len; ++i) { // Itera pelos bytes selecionados.
    ss << std::hex << std::setfill('0') << std::setw(2) // Formatação para 2 dígitos hex.
       << (int)(unsigned char)data[i] << " "; // Converte byte para inteiro sem sinal e escreve.
    if ((i + 1) % 16 == 0) ss << "\n                "; // Quebra linha a cada 16 bytes para legibilidade.
  }
  if (data.size() > max_bytes) ss << "..."; // Indica que houve truncamento.
  return ss.str(); // Retorna a string final hex.
}

GameServer::GameServer() { // Construtor: configura endpoint websocket e registra handlers.
  // set up access channels to only log interesting things
  endpoint.clear_access_channels(alevel::all); // Limpa canais de log anteriores.
  endpoint.set_access_channels(alevel::access_core); // Habilita log “core” do websocket.
  endpoint.set_access_channels(alevel::app); // Habilita log da aplicação (uso/erros/estado).

  // Initialize the Asio transport policy
  endpoint.init_asio(); // Inicializa o transporte ASIO.
  endpoint.set_reuse_addr(true); // Permite reutilizar endereço/porta em reinícios.

  // Bind the handlers we are using
  endpoint.set_socket_init_handler(bind(&GameServer::on_socket_init, this, ::_1, ::_2)); // Registra handler para inicializar socket.

  endpoint.set_open_handler(bind(&GameServer::on_open, this, _1)); // Handler ao abrir conexão.
  endpoint.set_message_handler(bind(&GameServer::on_message, this, _1, _2)); // Handler ao receber mensagem.
  endpoint.set_close_handler(bind(&GameServer::on_close, this, _1)); // Handler ao fechar conexão.
}

int GameServer::Run(IncomingConfig in_config) {
  endpoint.get_alog().write(alevel::app, "Running slither server on port " + std::to_string(in_config.port));

  config = in_config;
  PrintWorldInfo();

  endpoint.listen(in_config.port);
  endpoint.start_accept();

  world.Init(in_config.world);
  init = BuildInitPacket();
  NextTick(GetCurrentTime());

  try {
    endpoint.get_alog().write(alevel::app, "Server started...");
    endpoint.run();
    return 0;
  } catch (websocketpp::exception const &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
}

void GameServer::PrintWorldInfo() {
  std::stringstream s;
  s << "World info = \n" << world;
  endpoint.get_alog().write(alevel::app, s.str());
}

void GameServer::NextTick(long last) {
  last_time_point = last;
  timer = endpoint.set_timer(
      std::max(0L, timer_interval_ms - (GetCurrentTime() - last)),
      bind(&GameServer::on_timer, this, _1));
}

void GameServer::on_timer(error_code const &ec) {
  const long now = GetCurrentTime();
  const long dt = now - last_time_point;
  
  std::lock_guard<std::mutex> lock(game_mutex);

  if (ec) {
    endpoint.get_alog().write(alevel::app,
        "Main game loop timer error: " + ec.message());
    return;
  }

  world.Tick(dt);

  // --- Bot Spawning ---
  if (config.world.bot_respawn) {
      int active_bots = 0;
      for (auto &pair : world.GetSnakes()) {
          if (pair.second->bot && !(pair.second->update & (change_dying | change_dead))) {
              active_bots++;
          }
      }
      if (active_bots < config.world.bots) {
           SpawnBot();
      }
  }

  // --- FIX: Faster Spawn Animation ---
  for (auto &pair : world.GetSnakes()) {
      Snake *s = pair.second.get();
      
      // If snake is smaller than target size (spawning phase)
      if (s->parts.size() < s->target_score) {
          // Increase growth rate: 
          // 100 fullness = 1 body part.
          // 50 fullness/tick = 1 part every 2 ticks (16ms).
          // This makes the spawn animation fast and snappy like the original.
          s->IncreaseSnake(50); 
          s->update |= change_fullness | change_pos;
      }
  }

  BroadcastDebug();
  BroadcastUpdates();
  RemoveDeadSnakes();

  CleanupDeadSessions();

  // Broadcast Leaderboard (Every 2 seconds)
  if (now - last_leaderboard_time > 2000) {
      BroadcastLeaderboard();
      last_leaderboard_time = now;
  }

  // Broadcast Minimap (Every 1 second)
  if (now - last_minimap_time > 1000) {
      BroadcastMinimap();
      last_minimap_time = now;
  }

  const long step_time = GetCurrentTime() - now;
  if (step_time > 10) {
    endpoint.get_alog().write(alevel::app,
        "Load is too high, step took " + std::to_string(step_time) + "ms");
  }

  NextTick(now);
}

void GameServer::BroadcastDebug() {
  packet_debug_draw draw;

  // O círculo do battledome precisa aparecer sempre (define a regra de morte ao sair).

  // Battledome (anel verde no centro) — desenha mesmo que nenhum snake tenha mudado.
  // Isso ajuda a client-side map overlay a “aparecer” com o mesmo raio do servidor.
  {
    const uint16_t sis = 60000; // Separador numérico para debug circles (evita colisão com ids do snake).
    draw.circles.push_back(
        d_draw_circle{sis,
                      {static_cast<float>(WorldConfig::game_radius),
                       static_cast<float>(WorldConfig::game_radius)},
                      static_cast<float>(WorldConfig::battledome_radius),
                      0x00ff00});
  }

  for (Snake *s : world.GetChangedSnakes()) {
    uint16_t sis = static_cast<uint16_t>(s->id * 1000);

    // bound box
    draw.circles.push_back(
        d_draw_circle{sis++, {s->sbb.x, s->sbb.y}, s->sbb.r, 0xc8c8c8});

    // body inner circles
    const float r1 = s->get_snake_body_part_radius();

    draw.circles.push_back(
        d_draw_circle{sis++, {s->get_head_x(), s->get_head_y()}, r1, 0xc80000});

    const Body &sec = *(s->parts.begin() + 1);
    draw.circles.push_back(d_draw_circle{sis++, {sec.x, sec.y}, r1, 0x3c3c3c});
    draw.circles.push_back(
        d_draw_circle{sis++,
                      {sec.x + (s->get_head_x() - sec.x) / 2.0f,
                       sec.y + (s->get_head_y() - sec.y) / 2.0f},
                      r1,
                      0x646464});
    draw.circles.push_back(d_draw_circle{
        sis++, {s->parts.back().x, s->parts.back().y}, r1, 0x646464});

    // bounds
    for (const Sector *ss : s->sbb.sectors) {
      draw.circles.push_back(
          d_draw_circle{sis++, {ss->box.x, ss->box.y}, ss->box.r, 0x511883});
    }

    // intersection algorithm
    static const size_t head_size = 8;
    static const size_t tail_step = static_cast<size_t>(
        WorldConfig::sector_size / Snake::tail_step_distance);
    static const size_t tail_step_half = tail_step / 2;
    const size_t len = s->parts.size();

    if (len <= head_size + tail_step) {
      for (const Body &b : s->parts) {
        draw.circles.push_back(d_draw_circle{
            sis++, {b.x, b.y}, WorldConfig::move_step_distance, 0x646464});
      }
    } else {
      auto p = s->parts[3];
      draw.circles.push_back(d_draw_circle{
          sis++, {p.x, p.y}, WorldConfig::sector_size / 2, 0x848484});
      p = s->parts[0];
      draw.circles.push_back(d_draw_circle{
          sis++, {p.x, p.y}, WorldConfig::move_step_distance, 0x646464});
      p = s->parts[8];
      draw.circles.push_back(d_draw_circle{
          sis++, {p.x, p.y}, WorldConfig::move_step_distance, 0x646464});

      auto end = s->parts.end();
      for (auto i = s->parts.begin() + 7 + tail_step_half; i < end; i += tail_step) {
        draw.circles.push_back(d_draw_circle{
            sis++, {i->x, i->y}, WorldConfig::sector_size / 2, 0x848484});
      }
    }
  }

  if (!draw.empty()) {
    broadcast_debug(draw);
  }
}

// ----------------------------------------------------------------------------
// UPDATED SendFoodUpdate (Hybrid Protocol Support)
// ----------------------------------------------------------------------------
void GameServer::SendFoodUpdate(Snake *ptr) {
  // 1. Handle Eaten Food
  if (!ptr->eaten.empty()) {
    const snake_id_t id = ptr->id;

    for (auto &s : sessions) {
        if (s.second.snake_id == 0) continue;

        // Get the specific version for this player
        uint8_t ver = s.second.protocol_version;
        
        for (const auto &f : ptr->eaten) {
            // Send constructed packet
            endpoint.send_binary(s.first, packet_eat_food(id, Food(f.x, f.y, f.size, f.color), ver));
        }
    }
    ptr->eaten.clear();
  }

  // 2. Handle Spawned Food
  if (!ptr->spawn.empty()) {
    for (auto &s : sessions) {
        if (s.second.snake_id == 0) continue;
        bool is_modern = s.second.is_modern_protocol();
        for (const Food &f : ptr->spawn) {
             endpoint.send_binary(s.first, packet_spawn_food(f, is_modern));
        }
    }
    ptr->spawn.clear();
  }
}

// ----------------------------------------------------------------------------
// NEW CleanupDeadSessions
// ----------------------------------------------------------------------------
void GameServer::CleanupDeadSessions() {
    const long now = GetCurrentTime();
    std::vector<connection_hdl> to_close;
    
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        Session &ss = it->second;

        if (ss.death_timestamp > 0) {
            // After 2 seconds, kick the client.
            if (now - ss.death_timestamp > 2000) {
                to_close.push_back(it->first);
                
                // CRITICAL FIX: Mark ID as 0 immediately.
                // This prevents BroadcastLeaderboard/Minimap from trying 
                // to send data to this socket while it's closing, 
                // which stops the "invalid state" errors.
                ss.snake_id = 0; 
            }
        }
    }

    // Close connections safely
    for (connection_hdl hdl : to_close) {
        error_code ec;
        endpoint.close(hdl, websocketpp::close::status::normal, "Game Over", ec);
    }
}

// ----------------------------------------------------------------------------
// UPDATED BroadcastUpdates
// ----------------------------------------------------------------------------
void GameServer::BroadcastUpdates() {
  // Use a copy to safely iterate if map changes (though removal is deferred to RemoveDeadSnakes)
  auto changed_snakes = world.GetChangedSnakes();

  for (auto ptr : changed_snakes) {
    if (!ptr) continue;
    const snake_id_t id = ptr->id;
    const uint8_t flags = ptr->update;

    if (flags & change_dead) continue;

    if (flags & change_dying) {
      endpoint.get_alog().write(alevel::app, "Snake died: " + std::to_string(id));

      // 1. Spawn Food (CRITICAL: Must be first so clients see the food generated)
      if (world.GetSnake(id) != world.GetSnakes().end()) {
          ptr->on_dead_food_spawn(&world.GetSectors(), [&]() -> float {
            return world.NextRandomf();
          });
          SendFoodUpdate(ptr);
      }

      // 2. Broadcast Removal ('s') to EVERYONE.
      // Status 1 = Died (Explosion Animation).
      broadcast_binary(packet_remove_snake(id, packet_remove_snake::status_snake_died));

      // 3. Send Game Over ('v') ONLY to the victim.
      // This is sent last because the client might disconnect immediately upon receiving 'v'.
      if (!ptr->bot) {
        const auto ses_i = LoadSessionIter(id);
        if (ses_i != sessions.end()) {
          try {
            send_binary(ses_i, packet_end(packet_end::status_death));
            ses_i->second.death_timestamp = GetCurrentTime();
          } catch (...) {
             // Swallow errors if client disconnected
          }
        }
      }

      // 4. Mark Dead (Logic only)
      ptr->update |= change_dead;
      world.GetDead().push_back(id); 

      continue;
    }

    if (flags) {
      if (flags & (change_angle | change_speed)) {
        packet_rotation rot = packet_rotation();
        rot.snakeId = id;
        if (flags & change_angle) {
          ptr->update ^= change_angle;
          rot.ang = ptr->angle;
          if (flags & change_wangle) {
            ptr->update ^= change_wangle;
            rot.wang = ptr->wangle;
          }
        }
        if (flags & change_speed) {
          ptr->update ^= change_speed;
          rot.snakeSpeed = ptr->speed / 32.0f;
        }
        broadcast_binary(rot);
      }

      if (flags & change_pos) {
        ptr->update ^= change_pos;
        if (ptr->clientPartsIndex < ptr->parts.size()) {
          broadcast_binary(packet_inc(ptr));
          ptr->clientPartsIndex++;
        } else {
          if (ptr->clientPartsIndex > ptr->parts.size()) {
            broadcast_binary(packet_remove_part(ptr));
            ptr->clientPartsIndex--;
          }
          broadcast_binary(packet_move(ptr));
        }

        SendFoodUpdate(ptr);
        
        if (!ptr->bot && !(ptr->update & change_dying)) {
          const auto ses_i = LoadSessionIter(id);
          if (ses_i != sessions.end() && ses_i->second.death_timestamp == 0) {
              SendPOVUpdateTo(ses_i, ptr);
              if (flags & change_fullness) {
                send_binary(ses_i, packet_fullness(ptr));
                ptr->update ^= change_fullness;
              }
          }
        }
      }
    }
  }

  world.FlushChanges();
}

void GameServer::BroadcastLeaderboard() {
  // 1. Collect all snakes
  std::vector<std::shared_ptr<Snake>> sorted_snakes;
  for (auto &pair : world.GetSnakes()) {
    sorted_snakes.push_back(pair.second);
  }

  // 2. Sort descending by Score (Length + Fullness)
  std::sort(sorted_snakes.begin(), sorted_snakes.end(), 
      [](const std::shared_ptr<Snake>& a, const std::shared_ptr<Snake>& b) {
          return a->get_snake_score() > b->get_snake_score();
  });

  // 3. Prepare the Top 10 list (Base Packet)
  packet_leaderboard lb_base;
  lb_base.players = static_cast<uint16_t>(sorted_snakes.size());
  
  size_t top_count = std::min((size_t)10, sorted_snakes.size());
  for(size_t i = 0; i < top_count; i++) {
      lb_base.top.push_back(sorted_snakes[i]);
  }

  // 4. Send to each player individually using Iterator
  for (auto it = sessions.begin(); it != sessions.end(); ++it) {
      Session &sess = it->second;

      // Skip players who haven't spawned yet (snake_id 0)
      if (sess.snake_id == 0) continue;

      // Find this player's rank
      uint16_t my_rank = 0;
      for(size_t i = 0; i < sorted_snakes.size(); i++) {
          if(sorted_snakes[i]->id == sess.snake_id) {
              my_rank = i + 1;
              break;
          }
      }

      // Copy base packet and add specific rank data
      packet_leaderboard lb_packet = lb_base;
      lb_packet.local_rank = my_rank;
      // leaderboard_rank is usually 0 unless you are IN the top 10
      lb_packet.leaderboard_rank = (my_rank <= 10) ? (uint8_t)my_rank : 0;

      // FIX: Pass the iterator 'it' to send_binary
      send_binary(it, lb_packet);
  }
}

void GameServer::BroadcastMinimap() {
  // 1. Define Map Grid Size
  // Original is 80. You can increase this (e.g. 144) for C clients if desired,
  // but JS clients strictly expect 80x80 data in 'u' packets.
  const uint16_t map_dim = 144;
  
  std::vector<uint8_t> grid(map_dim * map_dim, 0);

  // 2. Map World Coordinates to Grid Coordinates
  float scale = (float)map_dim / (WorldConfig::game_radius * 2.0f);

  for (auto &pair : world.GetSnakes()) {
    Snake* s = pair.second.get();
    if (s->parts.empty() || (s->update & change_dead)) continue;

    for (size_t i = 0; i < s->parts.size(); i += 4) {
      const Body& b = s->parts[i];
      int mx = static_cast<int>(b.x * scale);
      int my = static_cast<int>(b.y * scale);
      if (mx >= 0 && mx < map_dim && my >= 0 && my < map_dim) {
          grid[my * map_dim + mx] = 1; 
      }
    }
  }

  // ---------------------------------------------------------
  // A. Build Forward Packet ('u') for JS Clients
  // ---------------------------------------------------------
  packet_minimap packet_fwd(map_dim);
  packet_fwd.packet_type = packet_t_minimap_legacy; // 'u'
  
  int skip = 0;
  for (size_t i = 0; i < grid.size(); ) {
    if (grid[i] == 0) {
      skip++;
      if (skip >= 127) {
        packet_fwd.data.push_back(static_cast<uint8_t>(128 + skip));
        skip = 0;
      }
      i++;
    } else {
      if (skip > 0) {
        packet_fwd.data.push_back(static_cast<uint8_t>(128 + skip));
        skip = 0;
      }
      uint8_t chunk = 0;
      for (int bit = 0; bit < 7; ++bit) {
        if (i + bit < grid.size() && grid[i + bit] != 0) chunk |= (1 << (6 - bit));
      }
      packet_fwd.data.push_back(chunk);
      i += 7;
    }
  }
  if (skip > 0) packet_fwd.data.push_back(static_cast<uint8_t>(128 + skip));

  // ---------------------------------------------------------
  // B. Build Reverse Packet ('M') for C Clients
  // ---------------------------------------------------------
  packet_minimap packet_rev(map_dim);
  packet_rev.packet_type = packet_t_minimap; // 'M'
  
  skip = 0;
  // Iterate backwards!
  for (int i = (map_dim * map_dim) - 1; i >= 0; ) {
    if (grid[i] == 0) {
      skip++;
      // Cap at 126 for C client safety (128+126 = 254 < 255)
      if (skip >= 126) {
        packet_rev.data.push_back(static_cast<uint8_t>(128 + skip));
        skip = 0;
      }
      i--;
    } else {
      if (skip > 0) {
        packet_rev.data.push_back(static_cast<uint8_t>(128 + skip));
        skip = 0;
      }
      uint8_t chunk = 0;
      // Pack bits backwards
      for (int bit = 0; bit < 7; ++bit) {
        if (i - bit >= 0 && grid[i - bit] != 0) chunk |= (1 << (6 - bit));
      }
      packet_rev.data.push_back(chunk);
      i -= 7;
    }
  }
  if (skip > 0) packet_rev.data.push_back(static_cast<uint8_t>(128 + skip));

  // ---------------------------------------------------------
  // C. Send appropriate packet to each session
  // ---------------------------------------------------------
  const long now = GetCurrentTime();
  for (auto it = sessions.begin(); it != sessions.end(); ++it) {
      if (it->second.snake_id == 0) continue;
      
      const uint16_t interval = static_cast<uint16_t>(now - it->second.last_packet_time);
      it->second.last_packet_time = now;

      if (it->second.is_modern_protocol()) {
          packet_rev.client_time = interval;
          endpoint.send_binary(it->first, packet_rev);
      } else {
          packet_fwd.client_time = interval;
          endpoint.send_binary(it->first, packet_fwd);
      }
  }
}

// ----------------------------------------------------------------------------
// UPDATED SendPOVUpdateTo (Hybrid Protocol Support)
// ----------------------------------------------------------------------------
void GameServer::SendPOVUpdateTo(SessionIter ses_i, Snake *ptr) {
  bool is_modern = ses_i->second.is_modern_protocol();

  if (!ptr->vp.new_sectors.empty()) {
    for (const Sector *s_ptr : ptr->vp.new_sectors) {
      send_binary(ses_i, packet_add_sector(s_ptr->x, s_ptr->y));
      
      // HYBRID CHECK
      if (is_modern) {
          send_binary(ses_i, packet_set_food_rel(&s_ptr->food));
      } else {
          send_binary(ses_i, packet_set_food_abs(&s_ptr->food));
      }
    }
    ptr->vp.new_sectors.clear();
  }

  if (!ptr->vp.old_sectors.empty()) {
    for (const Sector *s_ptr : ptr->vp.old_sectors) {
      send_binary(ses_i, packet_remove_sector(s_ptr->x, s_ptr->y));
    }
    ptr->vp.old_sectors.clear();
  }
}

void GameServer::RemoveDeadSnakes() {
  for (auto id : world.GetDead()) {
    RemoveSnake(id);
  }

  world.GetDead().clear();
}

void GameServer::on_socket_init(websocketpp::connection_hdl, boost::asio::ip::tcp::socket &s) {
  boost::asio::ip::tcp::no_delay option(true);
  s.set_option(option);
}

void GameServer::on_open(connection_hdl hdl) {
  std::lock_guard<std::mutex> lock(game_mutex);
  sessions[hdl] = Session(0, GetCurrentTime());
}

void GameServer::on_message(connection_hdl hdl, message_ptr ptr) {
  std::lock_guard<std::mutex> lock(game_mutex);
  if (ptr->get_opcode() != opcode::binary) {
    endpoint.get_alog().write(alevel::app,
        "Unknown incoming message opcode " + std::to_string(ptr->get_opcode()));
    return;
  }

  const size_t len = ptr->get_payload().size();
  if (len > 255) {
    endpoint.get_alog().write(alevel::app,
        COLOR_RED "Packet too big " + std::to_string(len) + COLOR_RESET);
    return;
  }

  if (len == 24) {
      endpoint.get_alog().write(alevel::app, 
          COLOR_YELLOW "    → Challenge response accepted" COLOR_RESET);
      return; 
  }

  std::stringstream buf(ptr->get_payload(), std::ios_base::in);

  in_packet_t packet_type = in_packet_t_angle;
  buf.read(reinterpret_cast<char*>(&packet_type), 1);

  const auto ses_i = sessions.find(hdl);
  if (ses_i == sessions.end()) {
    endpoint.get_alog().write(alevel::app, "No session, skip packet");
    return;
  }

  Session &ss = ses_i->second;

  if (ss.death_timestamp > 0) {
      // Allow only the 's' packet (Respawn) or Ping, block others
      std::stringstream temp_buf(ptr->get_payload());
      in_packet_t temp_type;
      temp_buf.read(reinterpret_cast<char*>(&temp_type), 1);
      
      // Only allow respawn ('s') or ping (251). Block steering/speed.
      if (temp_type != in_packet_t_username_skin && temp_type != in_packet_t_ping) {
          return;
      }
  }

  if (packet_type <= 250 && len == 1 && 
      packet_type != in_packet_t_start_login && 
      packet_type != in_packet_t_username_skin) {
    const float angle = Math::f_pi * packet_type / 125.0f;
    DoSnake(ss.snake_id, [=](Snake *s) {
      s->wangle = angle;
      s->update |= change_wangle;
    });
    return;
  }

  switch (packet_type) {
    case in_packet_t_ping:
      send_binary(ses_i, packet_pong());
      break;

    case in_packet_t_start_login:
        send_binary(ses_i, packet_pre_init()); 
        break;

    case in_packet_t_username_skin: // 's'
    {
        if (len < 3) return;
        uint8_t proto_ver = 0;
        buf.read(reinterpret_cast<char*>(&proto_ver), 1);
        ss.protocol_version = proto_ver; 

        if (ss.is_modern_protocol()) {
             char dummy[2];
             buf.read(dummy, 2); // Skip '333'
             endpoint.get_alog().write(alevel::app, "Detected Modern/C Client");
        } else {
             endpoint.get_alog().write(alevel::app, "Detected Legacy/JS Client");
        }

        buf.read(reinterpret_cast<char*>(&ss.skin), 1);
        
        uint8_t name_len = 0;
        buf.read(reinterpret_cast<char*>(&name_len), 1);

        ss.name.clear();
        if (name_len > 0) {
            long remaining = len - static_cast<long>(buf.tellg());
            if (name_len > remaining) name_len = static_cast<uint8_t>(remaining);
            if (name_len > 24) name_len = 24;
            if (name_len > 0) {
                std::vector<char> name_buf(name_len);
                buf.read(name_buf.data(), name_len);
                ss.name.assign(name_buf.data(), name_len);
            }
        }
        
        // --- FIX FOR SKIN RENDERING ---
        // Skip [0, 255] padding sent by C client after name
        if (ss.is_modern_protocol()) {
             char padding[2];
             // We use read to advance the buffer position
             if (len - buf.tellg() >= 2) {
                 buf.read(padding, 2);
             }
        }
        // ------------------------------

        ss.custom_skin_data.clear();
        if (buf.peek() != EOF) {
            long remaining_bytes = len - static_cast<long>(buf.tellg());
            if (remaining_bytes > 0) {
                std::vector<char> skin_buf(remaining_bytes);
                buf.read(skin_buf.data(), remaining_bytes);
                ss.custom_skin_data.assign(skin_buf.data(), remaining_bytes);
            }
        }

        std::stringstream connect_log;
        connect_log << COLOR_GREEN << "[CONNECT] " << COLOR_RESET
                    << "Name: '" << ss.name << "' "
                    << "| Skin ID: " << (int)ss.skin << " "
                    << "| Custom Skin Size: " << ss.custom_skin_data.size();
        endpoint.get_alog().write(alevel::app, connect_log.str());

        if (ss.snake_id == 0) {
            // Pass h_snake_start_score as target score
            const auto new_snake_ptr = world.CreateSnake(config.world.h_snake_start_score);
            new_snake_ptr->name = ss.name;
            new_snake_ptr->skin = ss.skin;
            new_snake_ptr->custom_skin_data = ss.custom_skin_data;

            world.AddSnake(new_snake_ptr);
            ss.snake_id = new_snake_ptr->id;
            connections[new_snake_ptr->id] = hdl;

            endpoint.send_binary(hdl, init);

            // Manual broadcast to handle version differences
            for (auto &s : sessions) {
                bool is_modern = s.second.is_modern_protocol();
                endpoint.send_binary(s.first, packet_add_snake(new_snake_ptr.get(), is_modern));
            }

            broadcast_binary(packet_move(new_snake_ptr.get()));
            SendPOVUpdateTo(ses_i, new_snake_ptr.get());

            for (auto snake_entry : world.GetSnakes()) {
                if (snake_entry.first != new_snake_ptr->id) {
                    const Snake *s = snake_entry.second.get();
                    bool is_modern = ss.is_modern_protocol(); 
                    send_binary(ses_i, packet_add_snake(s, is_modern));
                    send_binary(ses_i, packet_move(s));
                }
            }
        } else {
            DoSnake(ss.snake_id, [&ss](Snake *s) {
                s->name = ss.name;
                s->skin = ss.skin;
                s->custom_skin_data = ss.custom_skin_data;
            });
        }
    }
    break;

    case in_packet_t_rotation: 
        uint8_t rot_val;
        buf.read(reinterpret_cast<char*>(&rot_val), 1);
        if (rot_val < 128) {
             // Left
        } else {
             // Right
        }
        break;

    case in_packet_t_victory_message:
      buf >> packet_type;
      buf.str(ss.message);
      break;

    case in_packet_t_rot_left:
      buf >> packet_type; 
      break;

    case in_packet_t_rot_right:
      buf >> packet_type; 
      break;

    case in_packet_t_start_acc:
      DoSnake(ss.snake_id, [](Snake *s) { s->acceleration = true; });
      break;

    case in_packet_t_stop_acc:
      DoSnake(ss.snake_id, [](Snake *s) { s->acceleration = false; });
      break;

    default:
      endpoint.get_alog().write(alevel::app,
          "Unknown packet type " + std::to_string(packet_type) + ", len " +
          std::to_string(ptr->get_payload().size()));
      break;
  }
}

void GameServer::on_close(connection_hdl hdl) {
  std::lock_guard<std::mutex> lock(game_mutex);
  const auto ptr = sessions.find(hdl);
  if (ptr != sessions.end()) {
    const snake_id_t snakeId = ptr->second.snake_id;
    sessions.erase(ptr->first);
    RemoveSnake(snakeId);
  }
}

void GameServer::RemoveSnake(snake_id_t id) {
  connections.erase(id);
  world.RemoveSnake(id);
}

PacketInit GameServer::BuildInitPacket() {
  PacketInit init_packet;

  init_packet.game_radius = WorldConfig::game_radius;
  init_packet.max_snake_parts = WorldConfig::max_snake_parts;
  init_packet.sector_size = WorldConfig::sector_size;
  init_packet.sector_count_along_edge = WorldConfig::sector_count_along_edge;

  init_packet.spangdv = Snake::spangdv;
  init_packet.nsp1 = Snake::nsp1;
  init_packet.nsp2 = Snake::nsp2;
  init_packet.nsp3 = Snake::nsp3;

  init_packet.snake_ang_speed = 8.0f * Snake::snake_angular_speed / 1000.0f;
  init_packet.prey_ang_speed = 8.0f * Snake::prey_angular_speed / 1000.0f;
  init_packet.snake_tail_k = Snake::snake_tail_k;

  init_packet.protocol_version = WorldConfig::protocol_version;

  return init_packet;
}

long GameServer::GetCurrentTime() {
  using std::chrono::milliseconds;
  using std::chrono::duration_cast;
  using std::chrono::steady_clock;

  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void GameServer::DoSnake(snake_id_t id, std::function<void(Snake *)> f) {
  if (id > 0) {
    const auto snake_i = world.GetSnake(id);
    
    // --- SEGFAULT FIX ---
    // We MUST check if the snake was actually found before accessing it.
    // If snake_i equals world.GetSnakes().end(), the snake is gone/dead.
    if (snake_i != world.GetSnakes().end()) {
        f(snake_i->second.get());
    }
    // --------------------
  }
}

GameServer::SessionMap::iterator GameServer::LoadSessionIter(
    snake_id_t id) {
  const auto hdl_i = connections.find(id);
  if (hdl_i == connections.end()) {
    endpoint.get_alog().write(alevel::app,
        "Failed to locate snake connection " + std::to_string(id));
    return sessions.end();
  }

  const auto ses_i = sessions.find(hdl_i->second);
  if (ses_i == sessions.end()) {
    endpoint.get_alog().write(alevel::app,
        "Failed to locate snake session " + std::to_string(id));
  }

  return ses_i;
}

void GameServer::SpawnBot() {
  auto new_bot = world.CreateSnakeBot();
  world.AddSnake(new_bot);

  // Broadcast to all clients
  for (auto &s : sessions) {
    if (s.second.snake_id == 0) continue;
    bool is_modern = s.second.is_modern_protocol();
    endpoint.send_binary(s.first, packet_add_snake(new_bot.get(), is_modern));
  }
  broadcast_binary(packet_move(new_bot.get()));
}
