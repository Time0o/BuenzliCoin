#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

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
                   std::string &websocket_host,
                   uint16_t &websocket_port,
                   std::string &http_host,
                   uint16_t &http_port,
                   std::string &blockchain,
                   std::string &configuration,
                   bool verbose)
{
  po::variables_map vs;

  po::options_description options { "Node options" };
  options.add_options()
    ("name", po::value<std::string>(&name)->default_value("BuenzliNode"), "node name")
    ("websocket-host", po::value<std::string>(&websocket_host)->default_value("127.0.0.1"), "websocket server ip")
    ("websocket-port", po::value<uint16_t>(&websocket_port)->default_value(8332), "websocket server port")
    ("http-host", po::value<std::string>(&http_host)->default_value("127.0.0.1"), "http server ip")
    ("http-port", po::value<uint16_t>(&http_port)->default_value(8333), "http server port")
    ("blockchain", po::value<std::string>(&blockchain)->default_value(""), "persisted blockchain file")
    ("config", po::value<std::string>(&configuration)->default_value(""), "configuration file")
    ("verbose", po::bool_switch(&verbose)->default_value(false), "verbose log output");

  po::store(po::parse_command_line(argc, argv, options), vs);
  po::notify(vs);
}

std::unique_ptr<Node> node;

void create_node(std::string const &name,
                 std::string const &websocket_host,
                 uint16_t websocket_port,
                 std::string const &http_host,
                 uint16_t http_port,
                 Node::blockchain bc)
{
  node = std::make_unique<Node>(name,
                                websocket_host,
                                websocket_port,
                                http_host,
                                http_port,
                                std::move(bc));
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
    std::string websocket_host;
    uint16_t websocket_port;
    std::string http_host;
    uint16_t http_port;
    std::string blockchain;
    std::string configuration;
    bool verbose;

    parse_options(argc,
                  argv,
                  name,
                  websocket_host,
                  websocket_port,
                  http_host,
                  http_port,
                  blockchain,
                  configuration,
                  verbose);

    Node::blockchain bc;

    if (!blockchain.empty()) {
      std::ifstream f { blockchain };
      if (!f)
        throw std::runtime_error { "failed to open blockchain file" };

      std::stringstream ss;
      ss << f.rdbuf();

      bc = Node::blockchain::from_json(json::parse(ss.str()));
    }

    if (configuration.empty())
      config() = Config::from_defaults();
    else
      config() = Config::from_toml(configuration);

    log::init(verbose ? log::DEBUG : log::INFO);

    create_node(name,
                websocket_host,
                websocket_port,
                http_host,
                http_port,
                std::move(bc));

    nix::on_termination(stop_node);

    run_node();

  } catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
