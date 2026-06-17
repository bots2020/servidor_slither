#include "server/config.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

// bug clang++ 3.5 vs new gcc abi cant link against libboost build with gcc
// https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=797917
// http://stackoverflow.com/questions/34387406/boost-program-options-not-linking-correctly-under-clang
// http://developerblog.redhat.com/2015/02/05/gcc5-and-the-c11-abi/
IncomingConfig ParseCommandLine(const int argc, const char *const *argv) {
  IncomingConfig config;

  po::options_description generic("Generic options");
  generic.add_options()("help,h", po::bool_switch(&config.help),
                        "print help message")(
      "verbose,v", po::bool_switch(&config.verbose), "set verbose output")(
      "version", po::bool_switch(&config.version), "show version information")(
      "port,p", po::value<uint16_t>(&config.port)->default_value(config.port),
      "bind port")("debug,d",
                   po::bool_switch(&config.debug)->default_value(config.debug),
                   "enable debug mode");

  po::options_description conf("Configuration");
    conf.add_options()
        ("bots,b",
         po::value<uint16_t>(&config.world.bots)->default_value(config.world.bots),
         "spawn bots on startup")
        ("h_score", po::value<uint16_t>(&config.world.h_snake_start_score)
                       ->default_value(config.world.h_snake_start_score),
         "human start score")
        ("b_score", po::value<uint16_t>(&config.world.b_snake_start_score)
                       ->default_value(config.world.b_snake_start_score),
         "bot start score")
        ("min_len", po::value<uint16_t>(&config.world.snake_min_length)
                       ->default_value(config.world.snake_min_length),
         "init snake min length")
         
        // --- NEW FOOD SETTINGS ---
        ("food_rate", po::value<uint16_t>(&config.world.food_spawn_rate)
                       ->default_value(config.world.food_spawn_rate),
         "food items to spawn per tick (default: 2)")
        ("prob_near", po::value<uint16_t>(&config.world.spawn_prob_near_snake)
                       ->default_value(config.world.spawn_prob_near_snake),
         "weight: target sector neighboring a snake (default: 25)")
        ("prob_on", po::value<uint16_t>(&config.world.spawn_prob_on_snake)
                       ->default_value(config.world.spawn_prob_on_snake),
         "weight: target sector containing a snake (default: 25)")
        ("prob_rand", po::value<uint16_t>(&config.world.spawn_prob_random)
                       ->default_value(config.world.spawn_prob_random),
         "weight: target completely random sector (default: 50)");

  po::options_description cmdline_options;
  cmdline_options.add(generic).add(conf);

  po::variables_map vm;

  try {
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);
  } catch (const po::unknown_option &e) {
    std::cerr << "error: " << e.what() << '\n';
    config.help = true;
  }

  if (config.help) {
    std::cerr << "Usage: slither_server [OPTIONS]\n";
    std::cerr << cmdline_options << '\n';
    exit(1);
  }

  return config;
}
