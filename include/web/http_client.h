#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <boost/beast/http.hpp>

#include "json.h"

namespace bm
{

class HTTPClient
{
  struct Context;
  struct Connection;

  static constexpr char const *USER_AGENT = "BuenzliCoin/0.0.1 HTTPClient";

public:
  using method = boost::beast::http::verb;
  using status = boost::beast::http::status;

  HTTPClient(std::string const &host, uint16_t port);
  ~HTTPClient();

  std::string host() const
  { return m_host; }

  uint16_t port() const
  { return m_port; }

  std::pair<status, std::string> send_sync(std::string const &target,
                                           method const &method,
                                           json const &data = {}) const;

private:
  std::string m_host;
  uint16_t m_port;

  std::unique_ptr<Context> m_context;
};

} // end namespace bm
