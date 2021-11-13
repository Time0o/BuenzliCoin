#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "json.h"

namespace bm
{

struct WebSocketConnection;

class WebSocketClient
{

public:
  WebSocketClient(std::string const &addr, uint16_t port);
  ~WebSocketClient();

  std::string addr() const
  { return m_addr; }

  uint16_t port() const
  { return m_port; }

  json send(json const &data) const;

private:
  std::string m_addr;
  uint16_t m_port;

  std::unique_ptr<WebSocketConnection> m_connection;
};

} // end namespace bm
