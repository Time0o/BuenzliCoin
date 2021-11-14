#pragma once

#include <chrono>
#include <cstdint>
#include <iostream> // XXX
#include <list>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "blockchain.h"
#include "http_server.h"
#include "json.h"
#include "websocket_client.h"
#include "websocket_server.h"

namespace bm
{

class Node
{
public:
  explicit Node(std::string const &websocket_addr,
                uint16_t websocket_port,
                std::string const &http_addr,
                uint16_t http_port)
  : m_websocket_server { websocket_addr, websocket_port },
    m_http_server { http_addr, http_port }
  {
    websocket_setup();
    http_setup();
  }

  void run()
  {
    m_websocket_server_thread = std::thread([this]{ m_websocket_server.run(); });
    m_websocket_server_thread.detach();

    m_http_server.run();
  }

private:
  void websocket_setup()
  {
    m_websocket_server.support("request-latest-block",
                               [this](json const &)
                               { return handle_request_latest_block(); });

    m_websocket_server.support("receive-latest-block",
                               [this](json const &data)
                               { return handle_receive_latest_block(data); });
  }

  void http_setup()
  {
    m_http_server.support("list-blocks",
                          HTTPServer::method::get,
                          [this](json const &)
                          { return handle_list_blocks(); });

    m_http_server.support("add-block",
                          HTTPServer::method::post,
                          [this](json const &data)
                          { return handle_add_block(data); });

    m_http_server.support("list-peers",
                          HTTPServer::method::get,
                          [this](json const &)
                          { return handle_list_peers(); });

    m_http_server.support("add-peer",
                          HTTPServer::method::post,
                          [this](json const &data)
                          { return handle_add_peer(data); });
  }

  std::pair<HTTPServer::status, json> handle_list_blocks() const
  {
    json answer = m_blockchain.to_json();

    return { HTTPServer::status::ok, answer };
  }

  std::pair<HTTPServer::status, json> handle_add_block(json const &data)
  {
    // XXX Handle parse errors.
    m_blockchain.append(data["data"].get<std::string>());

    return { HTTPServer::status::ok, {} };
  }

  std::pair<HTTPServer::status, json> handle_list_peers() const
  {
    json answer = json::array();

    for (auto const &peer : m_websocket_peers)
      answer.push_back(peer.addr() + ":" + std::to_string(peer.port()));

    return { HTTPServer::status::ok, answer };
  }

  std::pair<HTTPServer::status, json> handle_add_peer(json const &data)
  {
    // XXX Handle parse errors.
    std::string addr { data["addr"].get<std::string>() };
    uint16_t port { data["port"].get<uint16_t>() };

    // XXX Handle connection failure
    m_websocket_peers.emplace_back(addr, port);

    request_latest_block(m_websocket_peers.back());

    return { HTTPServer::status::ok, {} };
  }

  void request_latest_block(WebSocketClient const &peer)
  {
    json request;
    request["target"] = "get-latest-block";

    peer.send_async(request,
                    [this](json const &data)
                    { handle_receive_latest_block(data); });
  }

  std::pair<bool, json> handle_request_latest_block() const
  {
    json answer;
    answer["status"] = "ok";
    // XXX Handle empty blockchain.
    answer["data"] = m_blockchain.begin()->to_json();

    return { true, answer };
  }

  std::pair<bool, json> handle_receive_latest_block(json const &data) const
  {
    // XXX Handle empty response.

    // XXX
    std::cout << "Received block: " << data << std::endl;

    return { true, {} };
  }

  Blockchain<> m_blockchain;

  WebSocketServer m_websocket_server;
  std::thread m_websocket_server_thread;
  std::list<WebSocketClient> m_websocket_peers;

  HTTPServer m_http_server;
};

} // end namespace bm
