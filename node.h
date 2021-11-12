#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "blockchain.h"
#include "http_server.h"

namespace bm
{

class Node
{
public:
  explicit Node(std::string const &addr, uint16_t port)
  : m_server { addr, port }
  {
    m_server.support("/list-blocks",
                     HTTPServer::method::get,
                     [this](json const &data)
                     { return handle_list_blocks(data); });

    m_server.support("/add-block",
                     HTTPServer::method::post,
                     [this](json const &data)
                     { return handle_add_block(data); });

  }

  void run()
  { m_server.run(); }

private:
  std::pair<HTTPServer::status, json> handle_list_blocks(json const &)
  {
    auto answer { m_blockchain.to_json() };

    return { HTTPServer::status::ok, answer };
  }

  std::pair<HTTPServer::status, json> handle_add_block(json const &data)
  {
    m_blockchain.append(data["data"].get<std::string>()); // XXX Handle parse errors.

    return { HTTPServer::status::ok, {} };
  }

  Blockchain<> m_blockchain;

  HTTPServer m_server;
};

} // end namespace bm
