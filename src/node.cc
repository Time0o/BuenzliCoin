#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "node.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

using namespace bm;

int main(int argc, char **argv)
{
  po::variables_map vs;

  try {
    po::options_description options { "Node options" };
    options.add_options()
      ("websocket-host", po::value<std::string>(), "websocket server ip")
      ("websocket-port", po::value<uint16_t>(), "websocket server port")
      ("http-host", po::value<std::string>(), "http server ip")
      ("http-port", po::value<uint16_t>(), "http server port");

    po::store(po::parse_command_line(argc, argv, options), vs);
    po::notify(vs);

  } catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
  }

  Node node {
    vs["websocket_host"].as<std::string>(),
    vs["websocket_post"].as<uint16_t>(),
    vs["http_host"].as<std::string>(),
    vs["http_post"].as<uint16_t>()
  };

  node.run();
}
