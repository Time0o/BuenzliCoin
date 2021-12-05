#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <utility>

#include "websocket_client.h"

namespace bc
{

class WebSocketPeer
{
public:
  WebSocketPeer(std::string const &host, uint16_t port)
  : m_client { host, port }
  {}

  std::string host() const
  { return m_client.host(); }

  uint16_t port() const
  { return m_client.port(); }

  void send(json request, WebSocketClient::callback cb) const
  {
    std::scoped_lock lock { m_mtx };

    m_client.send_async(std::move(request), std::move(cb));

    m_client.run();
  }

private:
  WebSocketClient m_client;
  mutable std::mutex m_mtx;
};

class WebSocketPeers
{
public:
  std::size_t size() const
  { return m_list.size(); }

  // XXX Handle connection failure
  std::size_t add(std::string const &host, uint16_t port)
  {
    std::scoped_lock lock { m_mtx };

    m_list.emplace_back(host, port);

    auto peer_id { m_list.size() };

    return peer_id;
  }

  std::size_t find(std::string const &host, uint16_t port) const
  {
    std::scoped_lock lock { m_mtx };

    for (std::size_t peer_id { 1 }; peer_id <= m_list.size(); ++peer_id) {
      auto const &peer { m_list[peer_id - 1] };

      if (peer.host() == host && peer.port() == port)
        return peer_id;
    }

    return 0;
  }

  void send(std::size_t peer_id, json request, WebSocketClient::callback cb) const
  {
    WebSocketPeer const *peer;

    {
      std::scoped_lock lock { m_mtx };

      peer = &m_list[peer_id - 1];
    }

    peer->send(std::move(request), std::move(cb));
  }

  json to_json() const
  {
    std::scoped_lock lock { m_mtx };

    json j;

    for (auto const &peer : m_list)
      j.push_back(peer.host() + ":" + std::to_string(peer.port()));

    return j;
  }

private:
  std::deque<WebSocketPeer> m_list;
  mutable std::mutex m_mtx;
};

} // end namespace bc
