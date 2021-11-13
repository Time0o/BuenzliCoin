#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "json.h"

namespace bm
{

class WebSocketServer
{
  friend class WebSocketSession;

public:
  using handler = std::function<std::pair<bool, json>(json const &)>;

  WebSocketServer(std::string const &addr, uint16_t port);

  void support(std::string const &target,
               handler handler);

  void run() const;

private:
  std::pair<bool, json> handle(std::string const &target,
                               json const &data) const;

  std::string m_addr;
  uint16_t m_port;

  std::unordered_map<std::string, handler> m_handlers;
  mutable std::mutex m_handlers_mtx;
};

} // end namespace bm
