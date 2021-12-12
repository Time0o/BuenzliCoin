#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>

#include "config.h"
#include "log.h"
#include "nix.h"
#include "node.h"

namespace po = boost::program_options;

using namespace bc;

namespace
{

void parse_options(int argc,
                   char **argv,
                   std::string &name,
                   std::string &config_,
                   std::string &websocket_host,
                   uint16_t &websocket_port,
                   std::string &http_host,
                   uint16_t &http_port,
                   bool verbose)
{
  po::variables_map vs;

  po::options_description options { "Node options" };
  options.add_options()
    ("name", po::value<std::string>(&name)->required(), "node name")
    ("config", po::value<std::string>(&config_)->required(), "configuration file")
    ("websocket-host", po::value<std::string>(&websocket_host)->required(), "websocket server ip")
    ("websocket-port", po::value<uint16_t>(&websocket_port)->required(), "websocket server port")
    ("http-host", po::value<std::string>(&http_host)->required(), "http server ip")
    ("http-port", po::value<uint16_t>(&http_port)->required(), "http server port")
    ("verbose", po::bool_switch(&verbose)->default_value(false), "verbose log output");

  po::store(po::parse_command_line(argc, argv, options), vs);
  po::notify(vs);
}

std::unique_ptr<Node> node;

void create_node(std::string const &name,
                 std::string const &websocket_host,
                 uint16_t websocket_port,
                 std::string const &http_host,
                 uint16_t http_port)
{
  node = std::make_unique<Node>(name,
                                websocket_host,
                                websocket_port,
                                http_host,
                                http_port);
}

void run_node()
{
  assert(node);

  node->run();
}

void stop_node(int)
{
  assert(node);

  node->stop();
}

} // end namespace

int main(int argc, char **argv)
{
  po::variables_map vs;

  try {
    std::string name;
    std::string config_;
    std::string websocket_host;
    uint16_t websocket_port;
    std::string http_host;
    uint16_t http_port;
    bool verbose;

    parse_options(argc,
                  argv,
                  name,
                  config_,
                  websocket_host,
                  websocket_port,
                  http_host,
                  http_port,
                  verbose);

    log::init(verbose ? log::DEBUG : log::INFO);

    config() = Config::from_toml(config_);

    create_node(name,
                websocket_host,
                websocket_port,
                http_host,
                http_port);

    nix::on_termination(stop_node);

    run_node();

  } catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
