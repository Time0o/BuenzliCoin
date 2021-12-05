#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "blockchain.h"
#include "json.h"
#include "log.h"
#include "uuid.h"
#include "web/http_error.h"
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
    m_log.info("Running 'add_block' handler");

    try {
      m_blockchain.append(data.get<std::string>());

    } catch (std::exception const &e) {
      std::string err { "Malformed 'add_block' request: '" + data.dump() + "'" };

      m_log.error(err);

      throw HTTPError { HTTPServer::status::bad_request, err };
    }

    exec_detached(&Node::broadcast_latest_block);

    return { HTTPServer::status::ok, {} };
  }

  std::pair<HTTPServer::status, json> handle_list_peers() const
  {
    m_log.info("Running 'list_peers' handler");

    return { HTTPServer::status::ok, m_websocket_peers.to_json() };
  }

  std::pair<HTTPServer::status, json> handle_add_peer(json const &data)
  {
    m_log.info("Running 'add_peer' handler");

    std::string host;
    uint16_t port;

    try {
      host = data["host"].get<std::string>();
      port = data["port"].get<uint16_t>();

    } catch (std::exception const &e) {
      std::string err { "Malformed 'add_peer' request: '" + data.dump() + "'" };

      m_log.error(err);

      throw HTTPError { HTTPServer::status::bad_request, err };
    }

    m_log.info("Peer is {}:{}", host, port);

    auto peer_id { m_websocket_peers.add(host, port) };

    exec_detached(&Node::request_latest_block, peer_id);

    return { HTTPServer::status::ok, {} };
  }

  json handle_request_latest_block(json const &data) const
  {
    m_log.info("Running 'request_latest_block' handler");

    if (m_blockchain.empty()) {
      std::string err { "Blockchain is empty" };

      m_log.error(err);

      throw WebSocketError(err);
    }

    json answer;
    answer["block"] = m_blockchain.latest_block().to_json();
    // XXX Client might see different host.
    answer["origin"]["host"] = m_websocket_server.host();
    answer["origin"]["port"] = m_websocket_server.port();

    return answer;
  }

  json handle_request_all_blocks(json const &data) const
  {
    m_log.info("Running 'request_all_blocks' handler");

    json answer;
    answer["blockchain"] = m_blockchain.to_json();
    // XXX Client might see different host.
    answer["origin"]["host"] = m_websocket_server.host();
    answer["origin"]["port"] = m_websocket_server.port();

    return answer;
  }

  json handle_receive_latest_block(json const &data)
  {
    m_log.info("Running 'receive_latest_block' handler");

    if (m_blockchain.empty())
      m_log.info("Blockchain is currently empty");
    else
      m_log.info("Current latest block: '{}'", m_blockchain.latest_block().to_json().dump());

    std::optional<Block<>> block;

    try {
      block = Block<>::from_json(data["block"]);

      if (!block->valid()) {
        std::string err { "Invalid block: '" + block->to_json().dump() + "'" };

        m_log.error(err);

        throw WebSocketError(err);
      }

      m_log.debug("Received block: '{}'", block->to_json().dump());

    } catch (std::exception const &e) {
      std::string err { "Malformed 'receive_latest_block' request: '" + data.dump() + "'" };

      m_log.error(err);

      throw WebSocketError(err);
    }

    if (block->index() > m_blockchain.length()) {
      std::string host;
      uint16_t port;

      try {
        host = data["origin"]["host"].get<std::string>();
        port = data["origin"]["port"].get<uint16_t>();

      } catch (std::exception const &e) {
        std::string err { "Invalid 'receive_latest_block' request: '" + data.dump() + "'" };

        m_log.error(err);

        throw WebSocketError(err);
      }

      auto peer_id { m_websocket_peers.find(host, port) };

      if (peer_id == 0)
        peer_id = m_websocket_peers.add(host, port);

      m_log.info("Peer is {}:{}", host, port);

      exec_detached(&Node::request_all_blocks, peer_id);

    } else if (block->index() == m_blockchain.length()) {
      if (m_blockchain.empty() && block->is_genesis()) {
        m_log.info("Appending new genesis block");
        m_blockchain.append(std::move(*block));

      } else if (block->is_successor_of(m_blockchain.latest_block())) {
        m_log.info("Appending next block");
        m_blockchain.append(std::move(*block));

      } else {
        m_log.info("Ignoring block (not a valid successor)");
      }

    } else {
      m_log.info("Ignoring block (not a successor)");
    }

    return {};
  }

  json handle_receive_all_blocks(json const &data)
  {
    m_log.info("Running 'receive_all_blocks' handler");

    std::optional<Blockchain<>> blockchain;

    try {
      blockchain = Blockchain<>::from_json(data["blockchain"]);

      if (!blockchain->valid()) {
        std::string err { "Invalid blockchain: '" + blockchain->to_json().dump() + "'" };

        m_log.error(err);

        throw WebSocketError(err);
      }

      m_log.debug("Received blockchain: '{}'", blockchain->to_json().dump());

    } catch (std::exception const &e) {
      std::string err { "Malformed 'receive_all_blocks' request: '" + data.dump() + "'" };

      m_log.error(err);

      throw WebSocketError(err);
    }

    if (blockchain->length() > m_blockchain.length()) {
      m_log.info("Replacing current blockchain");

      m_blockchain = std::move(*blockchain);
    }

    return {};
  }

  void broadcast_latest_block()
  {
    m_log.info("Broadcasting latest block");

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
          if (!success)
            m_log.error("Broadcasting latest block failed: {}", answer);
        });
    }
  }

  void request_latest_block(std::size_t peer_id)
  {
    m_log.info("Requesting latest block");

    json request;
    request["target"] = "/request-latest-block";

    m_websocket_peers.send(
      peer_id,
      request,
      [this](bool success, std::string const &answer)
      {
        if (success) {
          try {
            handle_receive_latest_block(json::parse(answer));
          } catch (...) {}

        } else {
          m_log.error("Requestion latest block failed: {}", answer);
        }
      });
  }

  void request_all_blocks(std::size_t peer_id)
  {
    m_log.info("Requesting all blocks");

    json request;
    request["target"] = "/request-all-blocks";

    m_websocket_peers.send(
      peer_id,
      request,
      [this](bool success, std::string const &answer)
      {
        if (success) {
          try {
            handle_receive_all_blocks(json::parse(answer));
          } catch (...) {}

        } else {
          m_log.error("Requesting all blocks failed");
        }
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
