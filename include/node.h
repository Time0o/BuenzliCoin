#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "blockchain.h"
#include "json.h"
#include "log.h"
#include "uuid.h"
#include "web/http_server.h"
#include "web/websocket_client.h"
#include "web/websocket_error.h"
#include "web/websocket_peer.h"
#include "web/websocket_server.h"

namespace bm
{

class Node
{
public:
  Node(std::string const &name,
       std::string const &websocket_addr,
       uint16_t websocket_port,
       std::string const &http_addr,
       uint16_t http_port)
  : m_name { name },
    m_log { "[{}] [{}]", m_name, m_uuid.to_string(true) },
    m_websocket_server { websocket_addr, websocket_port },
    m_http_server { http_addr, http_port }
  {
    websocket_setup();
    http_setup();
  }

  void run() const
  {
    m_log.info("Running node");

    std::thread websocket_server_thread([this]{ m_websocket_server.run(); });
    std::thread http_server_thread([this]{ m_http_server.run(); });

    websocket_server_thread.join();
    http_server_thread.join();
  }

  void stop() const
  {
    m_log.info("Stopping node");

    m_websocket_server.stop();
    m_http_server.stop();
  }

private:
  void websocket_setup()
  {
    m_websocket_server.support("/request-latest-block",
                               [this](json const &data)
                               { return handle_request_latest_block(data); });

    m_websocket_server.support("/request-all-blocks",
                               [this](json const &data)
                               { return handle_request_all_blocks(data); });

    m_websocket_server.support("/receive-latest-block",
                               [this](json const &data)
                               { return handle_receive_latest_block(data); });

    m_websocket_server.support("/receive-all-blocks",
                               [this](json const &data)
                               { return handle_receive_all_blocks(data); });
  }

  void http_setup()
  {
    m_http_server.support("/list-blocks",
                          HTTPServer::method::get,
                          [this](json const &)
                          { return handle_list_blocks(); });

    m_http_server.support("/add-block",
                          HTTPServer::method::post,
                          [this](json const &data)
                          { return handle_add_block(data); });

    m_http_server.support("/list-peers",
                          HTTPServer::method::get,
                          [this](json const &)
                          { return handle_list_peers(); });

    m_http_server.support("/add-peer",
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
    m_blockchain.append(data.get<std::string>());

    exec_detached(&Node::broadcast_latest_block);

    return { HTTPServer::status::ok, {} };
  }

  std::pair<HTTPServer::status, json> handle_list_peers() const
  {
    return { HTTPServer::status::ok, m_websocket_peers.to_json() };
  }

  std::pair<HTTPServer::status, json> handle_add_peer(json const &data)
  {
    // XXX Handle parse errors.
    std::string host { data["host"].get<std::string>() };
    uint16_t port { data["port"].get<uint16_t>() };

    auto peer_id { m_websocket_peers.add(host, port) };

    exec_detached(&Node::request_latest_block, peer_id);

    return { HTTPServer::status::ok, {} };
  }

  json handle_request_latest_block(json const &data) const
  {
    if (m_blockchain.empty())
      throw WebSocketError("Blockchain is empty");

    // XXX Handle failure.
    json answer;
    answer["block"] = m_blockchain.latest_block().to_json();
    // XXX Client might see different host.
    answer["origin"]["host"] = m_websocket_server.host();
    answer["origin"]["port"] = m_websocket_server.port();

    return answer;
  }

  json handle_request_all_blocks(json const &data) const
  {
    // XXX Handle failure.
    json answer;
    answer["blockchain"] = m_blockchain.to_json();
    // XXX Client might see different host.
    answer["origin"]["host"] = m_websocket_server.host();
    answer["origin"]["port"] = m_websocket_server.port();

    return answer;
  }

  json handle_receive_latest_block(json const &data)
  {
    // XXX Handle failures.
    auto block { Block<>::from_json(data["block"]) };
    if (!block.valid())
      // XXX Log this.
      return {};

    if (block.index() > m_blockchain.length()) {
      // XXX Handle failure.
      auto host { data["origin"]["host"].get<std::string>() };
      auto port { data["origin"]["port"].get<uint16_t>() };

      auto peer_id { m_websocket_peers.find(host, port) };

      if (peer_id == 0)
        peer_id = m_websocket_peers.add(host, port);

      exec_detached(&Node::request_all_blocks, peer_id);

    } else if ((m_blockchain.empty() && block.is_genesis()) ||
               block.is_successor_of(m_blockchain.latest_block())) {

      m_blockchain.append(std::move(block));
    }

    return {};
  }

  json handle_receive_all_blocks(json const &data)
  {
    // XXX Handle failure.
    auto blockchain { Blockchain<>::from_json(data["blockchain"]) };
    if (!blockchain.valid())
      // XXX Log this.
      return {};

    if (blockchain.length() > m_blockchain.length())
      m_blockchain = std::move(blockchain);

    return {};
  }

  void broadcast_latest_block()
  {
    json request;
    request["target"] = "/receive-latest-block";
    request["data"]["block"] = m_blockchain.latest_block().to_json();
    // XXX Client might see different host.
    request["data"]["origin"]["host"] = m_websocket_server.host();
    request["data"]["origin"]["port"] = m_websocket_server.port();

    for (std::size_t peer_id { 1 }; peer_id <= m_websocket_peers.size(); ++peer_id) {
      m_websocket_peers.send(
        peer_id,
        request,
        [this](bool success, std::string const &answer)
        {
          // XXX Handle failure.
        });
    }
  }

  void request_latest_block(std::size_t peer_id)
  {
    json request;
    request["target"] = "/request-latest-block";

    m_websocket_peers.send(
      peer_id,
      request,
      [this](bool success, std::string const &answer)
      {
        // XXX Handle failure.
        if (success)
            handle_receive_latest_block(json::parse(answer));
      });
  }

  void request_all_blocks(std::size_t peer_id)
  {
    json request;
    request["target"] = "/request-all-blocks";

    m_websocket_peers.send(
      peer_id,
      request,
      [this](bool success, std::string const &answer)
      {
        // XXX Handle failure.
        if (success)
            handle_receive_all_blocks(json::parse(answer));
      });
  }

  // XXX Use thread pool and join all threads before stopping.
  template<typename FUNC, typename ...ARGS>
  void exec_detached(FUNC func, ARGS... args)
  {
    std::thread t { [=, this]{ (this->*func)(args...); } };

    t.detach();
  }

  std::string const &m_name;
  uuid::UUID m_uuid;

  log::Logger m_log;

  Blockchain<> m_blockchain;
  mutable std::mutex m_blockchain_mtx;

  WebSocketServer m_websocket_server;
  WebSocketPeers m_websocket_peers;

  HTTPServer m_http_server;
};

} // end namespace bm
