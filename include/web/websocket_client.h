#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "json.h"

namespace bm
{

class WebSocketClient
{
  struct Context;
  class Connection;

  static constexpr char const *USER_AGENT = "BuenzliCoin/0.0.1 WebSocketClient";

public:
  using callback = std::function<void(bool, std::string const &)>;

  WebSocketClient(std::string const &host, uint16_t port);
  ~WebSocketClient();

  std::string host() const
  { return m_host; }

  uint16_t port() const
  { return m_port; }

  std::pair<bool, std::string> send_sync(json const &data) const;
  void send_async(json const &data, callback cb) const;

  void run() const;

private:
  std::string m_host;
  uint16_t m_port;

  std::unique_ptr<Context> m_context;
  std::shared_ptr<Connection> m_connection;
};

} // end namespace bm
