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

  json send(json const &data) const;

private:
  std::unique_ptr<WebSocketConnection> m_connection;
};

} // end namespace bm
