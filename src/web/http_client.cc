#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "json.h"
#include "web/http_client.h"

using namespace boost::asio;
using namespace boost::beast;

namespace bc
{

struct HTTPClient::Context
{
  Context(std::string const &host, uint16_t port)
  {
    ip::tcp::resolver resolver { ioc };
    server = { resolver.resolve(host, std::to_string(port)) };
  }

  io_context ioc { 1 };

  ip::tcp::resolver::results_type server;
};

struct HTTPClient::Connection
{
  Connection(HTTPClient::Context &context)
  : stream { context.ioc }
  { stream.connect(context.server); }

  ~Connection()
  { close(false); }

  void close(bool check=true)
  {
    error_code ec;

    stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);

    if (ec && ec != errc::not_connected) {
      if (check)
        throw system_error { ec };
      else
        std::cerr << ec.message() << std::endl;
    }
  }

  tcp_stream stream;
};

HTTPClient::HTTPClient(std::string const &host, uint16_t port)
: m_host { host },
  m_port { port },
  m_context { std::make_unique<Context>(host, port) }
{}

HTTPClient::~HTTPClient()
{}

std::pair<HTTPClient::status, std::string> HTTPClient::send_sync(
  std::string const &target,
  HTTPClient::method const &method,
  json const &data) const
{
  // Create connection.
  auto connection { std::make_unique<Connection>(*m_context) };

  // Send request.
  http::request<http::string_body> request { method, target, 11 };

  // Host
  request.set(http::field::host, m_host);
  // User agent.
  request.set(http::field::user_agent, USER_AGENT);

  // Body.
  if (!data.empty()) {
    request.set(http::field::content_type, "application/json");
    request.body() = data.dump();
  }

  request.prepare_payload();

  http::write(connection->stream, request);

  // Read response.
  http::response<http::string_body> response;

  flat_buffer buffer;
  http::read(connection->stream, buffer, response);

  // Close connection.
  connection->close();

  return { response.result(), response.body() };
}

} // end namespace bc
