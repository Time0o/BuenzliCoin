#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "json.h"

namespace bm
{

class WebSocketServer
{
  struct Context;
  class Connection;

  static constexpr char const *SERVER = "BuenzliCoin/0.0.1 WebSocketServer";

public:
  using handler = std::function<json(json const &)>;

  WebSocketServer(std::string const &host, uint16_t port);
  ~WebSocketServer();

  std::string host() const
  { return m_host; }

  uint16_t port() const
  { return m_port; }

  void support(std::string const &target, handler handler);

  void run() const;
  void stop() const;

private:
  std::pair<bool, json> handle(std::string const &target, json const &data) const;

  std::string m_host;
  uint16_t m_port;

  std::unordered_map<std::string, handler> m_handlers;
  mutable std::mutex m_handlers_mtx;

  std::unique_ptr<Context> m_context;
};

} // end namespace bm
