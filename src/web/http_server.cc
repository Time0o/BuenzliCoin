#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "json.h"
#include "web/http_error.h"
#include "web/http_server.h"

using namespace boost::asio;
using namespace boost::beast;

namespace bm {

struct HTTPServer::Context
{
  Context(std::string const &host, uint16_t port)
  : acceptor { ioc, { ip::make_address(host), port } }
  {}

  io_context ioc { 1 };

  ip::tcp::acceptor acceptor;
};

class HTTPServer::Connection : public std::enable_shared_from_this<Connection>
{
  static constexpr std::chrono::seconds TIMEOUT { 30 };

public:
  Connection(HTTPServer const *server, ip::tcp::socket &&socket)
  : m_server { server },
    m_stream { std::move(socket) }
  {}

  static std::shared_ptr<Connection> create(HTTPServer const *server,
                                            ip::tcp::socket &&socket)
  {
    return std::make_shared<Connection>(server, std::move(socket));
  }

  static void accept(HTTPServer const *server,
                     io_context &ioc,
                     ip::tcp::acceptor &acceptor)
  {
    acceptor.async_accept(
      make_strand(ioc),
      [server, &ioc, &acceptor](error_code ec, ip::tcp::socket socket)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          create(server, std::move(socket))->run();

        accept(server, ioc, acceptor);
      });
  }

  void run()
  {
    auto this_ { shared_from_this() };

    dispatch(
      m_stream.get_executor(),
      [this_]
      { this_->read_request(); });
  }

private:
  void read_request()
  {
    auto this_ { shared_from_this() };

    m_request = {};

    m_stream.expires_after(TIMEOUT);

    http::async_read(
      m_stream,
      m_buffer,
      m_request,
      [this_](error_code ec, std::size_t)
      {
        if (ec == http::error::end_of_stream) {
          this_->shutdown();
          return;
        }

        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->handle_request();
      });
  }

  void handle_request()
  {
    http::status result_status { http::status::ok };
    std::string result_content_type { "text/plain" };
    std::stringstream result_data;

    // Parse request.
    std::string target { m_request.target() };
    http::verb method { m_request.method() };

    auto request_body { m_request.body() };

    json data;
    if (!request_body.empty()) {
      auto content_type { m_request[http::field::content_type] };

      if (content_type == "application/json") {
        try {
          data = json::parse(request_body);
        } catch (json::parse_error const &e) {
          result_status = http::status::bad_request;
          result_data << "Failed to parse JSON: " << e.what();
        }
      } else {
        result_status = http::status::unsupported_media_type;
        result_data << "Unsupported media type '" << content_type << "'";
      }
    }

    // Handle request.
    if (result_status == http::status::ok) {
      json answer;
      try {
        std::tie(result_status, answer) = m_server->handle(target, method, data);
      } catch (HTTPError const &e) {
        result_status = e.status();
        result_data << e.what();
      }

      switch (result_status) {
        case http::status::ok:
          {
            result_content_type = "application/json";
            result_data << answer;
          }
          break;
        case http::status::not_found:
          {
            result_data << "File not found";
          }
          break;
        case http::status::bad_request:
          {
            auto method_string { m_request.method_string() };
            result_data << "Invalid request method '" << method_string << "'";
          }
          break;
        default:
          break;
      }
    }

    // Construct response.

    // Keep-alive.
    m_response.keep_alive(false);
    // Version.
    m_response.version(m_request.version());
    // Server.
    m_response.set(http::field::server, SERVER);
    // Status code.
    m_response.result(result_status);
    // Content-type
    m_response.set(http::field::content_type, result_content_type);
    // Body.
    ostream(m_response.body()) << result_data.str();
    m_response.content_length(m_response.body().size());

    m_response.prepare_payload();

    write_response();
  }

  void write_response()
  {
    auto this_ { shared_from_this() };

    http::async_write(
      m_stream,
      m_response,
      [this_](error_code ec, std::size_t)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;

        this_->shutdown();
      });
  }

  void shutdown()
  {
    error_code ec;
    m_stream.socket().shutdown(ip::tcp::socket::shutdown_send, ec);
  }

  HTTPServer const *m_server;

  tcp_stream m_stream;

  flat_buffer m_buffer;

  http::request<http::string_body> m_request;
  http::response<http::dynamic_body> m_response;
};

HTTPServer::HTTPServer(std::string const &host, uint16_t port)
: m_host { host },
  m_port { port },
  m_context { std::make_unique<Context>(host, port) }
{}

HTTPServer::~HTTPServer()
{}

void HTTPServer::support(std::string const &target,
                         method const &method,
                         handler handler)
{
  assert(!target.empty() && target.front() == '/');

  {
    std::scoped_lock lock(m_handlers_mtx);

    m_handlers[target].emplace_back(method, std::move(handler));
  }
}

std::pair<HTTPServer::status, json> HTTPServer::handle(
  std::string const &target,
  method const &method,
  json const &data) const
{
  std::scoped_lock lock(m_handlers_mtx);

  auto it { m_handlers.find(target) };
  if (it == m_handlers.end())
    return { status::not_found, {} };

  for (auto const &[method_, handler] : it->second) {
    if (method_ == method)
      return handler(data);
  }

  return { status::bad_request, {} };
}

void HTTPServer::run() const
{
  Connection::accept(this, m_context->ioc, m_context->acceptor);

  m_context->ioc.run();
}

void HTTPServer::stop() const
{
  m_context->ioc.stop();
}

} // end namespace bm
