#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/beast/http.hpp>

#include "json.h"

namespace bm
{

class HTTPServer
{
  friend class HTTPConnection;

public:
  using method = boost::beast::http::verb;
  using status = boost::beast::http::status;
  using handler = std::function<std::pair<status, json>(json const &)>;
  using handlers = std::vector<std::pair<method, handler>>;

  HTTPServer(std::string const &addr, uint16_t port);

  void support(std::string const &target,
               method const &method,
               handler handler);

  void run() const;

private:
  std::pair<status, json> handle(std::string const &target,
                                 method const &method,
                                 json const &data) const;

  std::string m_addr;
  uint16_t m_port;

  std::unordered_map<std::string, handlers> m_handlers;
  mutable std::mutex m_handlers_mtx;
};

} // end namespace bm