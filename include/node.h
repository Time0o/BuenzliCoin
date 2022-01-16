#pragma once

#include <cstdint>
#include <string>
#include <thread>
#include <utility>

#include "blockchain.h"
#include "json.h"
#include "log.h"
#include "text.h"
#include "transaction.h"
#include "uuid.h"
#include "web/http_server.h"
#include "web/websocket_peer.h"
#include "web/websocket_server.h"

namespace bc
{

class Node
{
#ifdef TRANSACTIONS
  using transaction = Transaction<>;
  using transaction_list = TransactionList<>;

  using block = Block<transaction_list>;
  using blockchain = Blockchain<transaction_list>;
#else
  using block = Block<Text>;
  using blockchain = Blockchain<Text>;
#endif // TRANSACTION

public:
  Node(std::string const &name,
       std::string const &websocket_addr,
       uint16_t websocket_port,
       std::string const &http_addr,
       uint16_t http_port);

  void run() const;
  void stop() const;

private:
  void websocket_setup();
  void http_setup();

  std::pair<HTTPServer::status, json> handle_blocks_get() const;
  std::pair<HTTPServer::status, json> handle_blocks_post(json const &data);
  std::pair<HTTPServer::status, json> handle_peers_get() const;
  std::pair<HTTPServer::status, json> handle_peers_post(json const &data);
#ifdef TRANSACTIONS
  std::pair<HTTPServer::status, json> handle_transactions_post(json const &data);
  std::pair<HTTPServer::status, json> handle_transactions_unconfirmed_get();
  std::pair<HTTPServer::status, json> handle_transactions_unspent_get() const;
#endif // TRANSACTIONS

  json handle_request_latest_block(json const &data) const;
  json handle_request_all_blocks(json const &data) const;
  json handle_receive_latest_block(json const &data);
  json handle_receive_all_blocks(json const &data);
#ifdef TRANSACTIONS
  json handle_receive_transaction(json const &data);
#endif // TRANSACTIONS

  void broadcast_latest_block();
  void request_latest_block(std::size_t peer_id);
  void request_all_blocks(std::size_t peer_id);
#ifdef TRANSACTIONS
  void broadcast_transaction(transaction const &t);
#endif // TRANSACTIONS

  // XXX Use thread pool and join all threads before stopping.
  template<typename FUNC, typename ...ARGS>
  void detach(FUNC func, ARGS... args)
  {
    std::thread t { [=, this]{ (this->*func)(args...); } };

    t.detach();
  }

  std::string const &m_name;
  uuid::UUID m_uuid;

  log::Logger m_log;

  blockchain m_blockchain;

#ifdef TRANSACTIONS
  TransactionUnspentOutputs<> m_transaction_unspent_outputs;
  TransactionUnconfirmedPool<> m_transaction_unconfirmed_pool;
#endif // TRANSACTIONS

  WebSocketServer m_websocket_server;
  WebSocketPeers m_websocket_peers;

  HTTPServer m_http_server;
};

} // end namespace bc
