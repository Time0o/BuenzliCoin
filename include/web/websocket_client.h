#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "json.h"

namespace bm
{

struct WebSocketConnection;

class WebSocketClient
{

public:
  using callback = std::function<void(json const &)>;

  WebSocketClient(std::string const &addr, uint16_t port);
  ~WebSocketClient();

  std::string addr() const
  { return m_addr; }

  uint16_t port() const
  { return m_port; }

  json send_sync(json const &data) const;
  void send_async(json const &data, callback const &cb) const;

private:
  std::string m_addr;
  uint16_t m_port;

  std::unique_ptr<WebSocketConnection> m_connection;
};

} // end namespace bm
