#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include "blockchain.h"
#include "config.h"
#include "json.h"
#include "log.h"
#include "node.h"
#include "uuid.h"
#include "web/http_error.h"
#include "web/http_server.h"
#include "web/websocket_error.h"
#include "web/websocket_peer.h"
#include "web/websocket_server.h"

namespace bc
{

Node::Node(std::string const &name,
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

void Node::run() const
{
  m_log.info("Running node");

  m_log.info("Websocket API reachable under {}:{}",
             m_websocket_server.host(), m_websocket_server.port());

  m_log.info("REST API reachable under {}:{}",
             m_http_server.host(), m_http_server.port());

  std::thread websocket_server_thread([this]{ m_websocket_server.run(); });
  std::thread http_server_thread([this]{ m_http_server.run(); });

  websocket_server_thread.join();
  http_server_thread.join();
}

void Node::stop() const
{
  m_log.info("Stopping node");

  m_websocket_server.stop();
  m_http_server.stop();
}

void Node::websocket_setup()
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

#ifdef TRANSACTIONS
  m_websocket_server.support("/receive-transaction",
                             [this](json const &data)
                             { return handle_receive_transaction(data); });
#endif // TRANSACTIONS
}

void Node::http_setup()
{
  m_http_server.support("/blocks",
                        HTTPServer::method::get,
                        [this](json const &)
                        { return handle_blocks_get(); });

  m_http_server.support("/blocks/latest",
                        HTTPServer::method::get,
                        [this](json const &)
                        { return handle_blocks_latest_get(); });

  m_http_server.support("/blocks",
                        HTTPServer::method::post,
                        [this](json const &data)
                        { return handle_blocks_post(data); });

  m_http_server.support("/peers",
                        HTTPServer::method::get,
                        [this](json const &)
                        { return handle_peers_get(); });

  m_http_server.support("/peers",
                        HTTPServer::method::post,
                        [this](json const &data)
                        { return handle_peers_post(data); });

#ifdef TRANSACTIONS
  m_http_server.support("/transactions/latest",
                        HTTPServer::method::get,
                        [this](json const &data)
                        { return handle_transactions_latest_get(data); });

  m_http_server.support("/transactions",
                        HTTPServer::method::post,
                        [this](json const &data)
                        { return handle_transactions_post(data); });

  m_http_server.support("/transactions/unconfirmed",
                        HTTPServer::method::get,
                        [this](json const &)
                        { return handle_transactions_unconfirmed_get(); });

  m_http_server.support("/transactions/unspent",
                        HTTPServer::method::get,
                        [this](json const &)
                        { return handle_transactions_unspent_get(); });
#endif // TRANSACTIONS
}

std::pair<HTTPServer::status, json> Node::handle_blocks_get() const
{
  m_log.info("Running 'GET /blocks' handler");

  json answer = m_blockchain.to_json();

  return { HTTPServer::status::ok, answer };
}

std::pair<HTTPServer::status, json> Node::handle_blocks_latest_get() const
{
  m_log.info("Running 'GET /blocks/latest' handler");

  json answer;
  if (!m_blockchain.empty())
    answer = m_blockchain.latest_block().to_json();

  return { HTTPServer::status::ok, answer };
}

std::pair<HTTPServer::status, json> Node::handle_blocks_post(json const &data)
{
  m_log.info("Running 'POST /blocks' handler");

  try {
#ifdef TRANSACTIONS

    auto reward_address { data.get<std::string>() };

    std::vector<transaction> ts_;

    m_log.info("Assembling block from unconfirmed transaction pool");

    ts_.push_back(transaction::reward(reward_address, m_blockchain.length()));

    for (std::size_t i { 0 }; i < config().transaction_num_per_block; ++i) {
      if (m_transaction_unconfirmed_pool.empty())
        break;

      ts_.push_back(m_transaction_unconfirmed_pool.next());
    }

    transaction_list ts { ts_.begin(), ts_.end() };

    m_log.info("Constructing block");

    m_blockchain.construct_next_block(ts);

    m_log.info("Updating unspent transaction outputs");

    for (auto const &t : ts.get())
      m_transaction_unspent_outputs.update(t);

    m_log.info("Updating unconfirmed transaction pool");

    m_transaction_unconfirmed_pool.prune(m_transaction_unspent_outputs.get());

#else

    auto d { block::data_type::from_json(data) };

    m_blockchain.construct_next_block(d);

#endif // TRANSACTIONS

    m_log.debug("Constructed next block: '{}'",
                 m_blockchain.latest_block().to_json().dump());

  } catch (std::exception const &e) {
    std::string err {
      "Malformed 'POST /blocks' request: '" + data.dump() + "': " + e.what() };

    m_log.error(err);

    throw HTTPError { HTTPServer::status::bad_request, err };
  }

  detach(&Node::broadcast_latest_block);

  return { HTTPServer::status::ok, {} };
}

std::pair<HTTPServer::status, json> Node::handle_peers_get() const
{
  m_log.info("Running 'GET /peers' handler");

  return { HTTPServer::status::ok, m_websocket_peers.to_json() };
}

std::pair<HTTPServer::status, json> Node::handle_peers_post(json const &data)
{
  m_log.info("Running 'POST /peers' handler");

  std::string host;
  uint16_t port;

  try {
    host = data["host"].get<std::string>();
    port = data["port"].get<uint16_t>();

  } catch (std::exception const &e) {
    std::string err {
      "Malformed 'POST /peers' request: '" + data.dump() + "': " + e.what() };

    m_log.error(err);

    throw HTTPError { HTTPServer::status::bad_request, err };
  }

  m_log.info("Peer is {}:{}", host, port);

  auto peer_id { m_websocket_peers.add(host, port) };

  detach(&Node::request_latest_block, peer_id);

  return { HTTPServer::status::ok, {} };
}

#ifdef TRANSACTIONS

std::pair<HTTPServer::status, json> Node::handle_transactions_latest_get(json const &data)
{
  m_log.info("Running 'GET /transactions/latest' handler");

  json answer;
  if (!m_blockchain.empty())
    answer = m_blockchain.latest_block().data().to_json();

  return { HTTPServer::status::ok, answer };
}

std::pair<HTTPServer::status, json> Node::handle_transactions_post(json const &data)
{
  m_log.info("Running 'POST /transactions' handler");

  try {
    auto t { transaction::from_json(data) };

    m_log.debug("Linking transaction with unspent transaction outputs");

    t.update_unspent_outputs(m_transaction_unspent_outputs.get());

    m_log.info("Adding transaction to unconfirmed transaction pool");

    m_transaction_unconfirmed_pool.add(std::move(t));

    broadcast_transaction(t);

  } catch (std::exception const &e) {
    std::string err {
      "Malformed 'POST /transactions/unconfirmed' request: '" + data.dump() + "': " + e.what() };

    m_log.error(err);

    throw HTTPError { HTTPServer::status::bad_request, err };
  }

  return { HTTPServer::status::ok, {} };
}

std::pair<HTTPServer::status, json> Node::handle_transactions_unconfirmed_get()
{
  m_log.info("Running 'GET /transactions/unconfirmed' handler");

  json answer = m_transaction_unconfirmed_pool.to_json();

  return { HTTPServer::status::ok, answer };
}

std::pair<HTTPServer::status, json> Node::handle_transactions_unspent_get() const
{
  m_log.info("Running 'GET /transactions/unspent' handler");

  json answer = m_transaction_unspent_outputs.to_json();

  return { HTTPServer::status::ok, answer };
}

#endif // TRANSACTIONS

json Node::handle_request_latest_block(json const &data) const
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

json Node::handle_request_all_blocks(json const &data) const
{
  m_log.info("Running 'request_all_blocks' handler");

  json answer;
  answer["blockchain"] = m_blockchain.to_json();
  // XXX Client might see different host.
  answer["origin"]["host"] = m_websocket_server.host();
  answer["origin"]["port"] = m_websocket_server.port();

  return answer;
}

json Node::handle_receive_latest_block(json const &data)
{
  m_log.info("Running 'receive_latest_block' handler");

  if (m_blockchain.empty())
    m_log.info("Blockchain is currently empty");
  else
    m_log.info("Current latest block: '{}'", m_blockchain.latest_block().to_json().dump());

  std::unique_ptr<block> b;

  try {
    b = std::make_unique<block>(block::from_json(data["block"]));

    m_log.debug("Received block: '{}'", b->to_json().dump());

#ifdef TRANSACTIONS
    auto &ts { b->data() };

    m_log.debug("Linking transactions with unspent transaction outputs");

    for (auto &t : ts.get())
      t.update_unspent_outputs(m_transaction_unspent_outputs.get());
#endif // TRANSACTIONS

    auto [b_valid, b_error] = b->valid();

    if (!b_valid) {
      std::string err { "Invalid block: '" + b->to_json().dump() + "': " + b_error };

      m_log.error(err);

      throw WebSocketError(err);
    }

    if (b->index() > m_blockchain.length()) {
      std::string host;
      uint16_t port;

      try {
        host = data["origin"]["host"].get<std::string>();
        port = data["origin"]["port"].get<uint16_t>();

      } catch (std::exception const &e) {
        std::string err {
          "Malformed 'receive_latest_block' request: '" + data.dump() + "'" + e.what() };

        m_log.error(err);

        throw WebSocketError(err);
      }

      auto peer_id { m_websocket_peers.find(host, port) };

      if (peer_id == 0)
        peer_id = m_websocket_peers.add(host, port);

      m_log.info("Peer is {}:{}", host, port);

      detach(&Node::request_all_blocks, peer_id);

    } else if (b->index() == m_blockchain.length()) {
      if ((m_blockchain.empty() && b->is_genesis()) ||
          (!m_blockchain.empty() && b->is_successor_of(m_blockchain.latest_block()))) {

        m_log.info("Appending next block");

        m_blockchain.append_next_block(*b);

      } else {
        m_log.info("Ignoring block (not a valid successor)");
        return {};
      }

    } else {
      m_log.info("Ignoring block (not a successor)");
      return {};
    }

  } catch (std::exception const &e) {
    std::string err {
      "Malformed 'receive_latest_block' request: '" + data.dump() + "': " + e.what() };

    m_log.error(err);

    throw WebSocketError(err);
  }

#ifdef TRANSACTIONS

  auto const &ts { b->data() };

  m_log.info("Updating unspent transaction outputs");

  for (auto const &t : ts.get())
    m_transaction_unspent_outputs.update(t);

  m_log.info("Updating unconfirmed transaction pool");

  for (auto const &t : ts.get())
    m_transaction_unconfirmed_pool.remove(t);

  m_transaction_unconfirmed_pool.prune(m_transaction_unspent_outputs.get());

#endif // TRANSACTIONS

  return {};
}

json Node::handle_receive_all_blocks(json const &data)
{
  m_log.info("Running 'receive_all_blocks' handler");

  std::unique_ptr<blockchain> bc;

  try {
    bc = std::make_unique<blockchain>(blockchain::from_json(data["blockchain"]));

    auto [bc_valid, bc_error] = bc->valid();

    if (!bc_valid) {
      std::string err { "Invalid blockchain: '" + bc->to_json().dump() + "': " + bc_error };

      m_log.error(err);

      throw WebSocketError(err);
    }

    m_log.debug("Received blockchain: '{}'", bc->to_json().dump());

  } catch (std::exception const &e) {
    std::string err {
      "Malformed 'receive_all_blocks' request: '" + data.dump() + "': " + e.what() };

    m_log.error(err);

    throw WebSocketError(err);
  }

  if (*bc > m_blockchain) {
    m_log.info("Replacing current blockchain");

    m_blockchain = std::move(*bc);
  }

  return {};
}

#ifdef TRANSACTIONS

json Node::handle_receive_transaction(json const &data)
{
  m_log.info("Running 'receive_transaction' handler");

  std::unique_ptr<transaction> t;

  try {
    t = std::make_unique<transaction>(transaction::from_json(data));

    t->update_unspent_outputs(m_transaction_unspent_outputs.get());

    m_log.debug("Received transaction: '{}'", t->to_json().dump());

  } catch (std::exception const &e) {
    std::string err {
      "Malformed 'receive_transaction' request: '" + data.dump() + "': " + e.what() };

    m_log.error(err);

    throw WebSocketError(err);
  }

  m_log.info("Adding transaction to unconfirmed transaction pool");

  try {
    m_transaction_unconfirmed_pool.add(*t);
  } catch (std::exception const &e) {
    m_log.error("Failed to add transaction to unconfirmed transaction pool: {}", e.what());
  }

  return {};
}

#endif // TRANSACTIONS

void Node::broadcast_latest_block()
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

void Node::request_latest_block(std::size_t peer_id)
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

void Node::request_all_blocks(std::size_t peer_id)
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

#ifdef TRANSACTIONS

void Node::broadcast_transaction(transaction const &t)
{
  m_log.info("Broadcasting transaction");

  json request;
  request["target"] = "/receive-transaction";
  request["data"] = t.to_json();

  for (std::size_t peer_id { 1 }; peer_id <= m_websocket_peers.size(); ++peer_id) {
    m_websocket_peers.send(
      peer_id,
      request,
      [this](bool success, std::string const &answer)
      {
        if (!success)
          m_log.error("Broadcasting transaction failed: {}", answer);
      });
  }
}

#endif // TRANSACTIONS

} // end namespace bc
