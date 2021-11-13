#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "http_server.h"
#include "json.h"

using namespace boost::asio;
using namespace boost::beast;

namespace bm {

class HTTPConnection : public std::enable_shared_from_this<HTTPConnection>
{
  static constexpr std::size_t BUFFER_SIZE { 8192 };
  static constexpr std::chrono::seconds PROCESS_TIMEOUT { 60 };

public:
  HTTPConnection(HTTPServer const *server, ip::tcp::socket &&socket)
  : m_server { server },
    m_socket { std::move(socket) },
    m_timer { m_socket.get_executor(), PROCESS_TIMEOUT }
  {}

  void run()
  {
    read_request();
    handle_timeout();
  }

private:
  void read_request()
  {
    auto this_ { shared_from_this() };

    http::async_read(
      m_socket,
      m_buffer,
      m_request,
      [this_](error_code ec, std::size_t)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->handle_request();
      });
  }

  void handle_request()
  {
    m_response.version(m_request.version());
    m_response.keep_alive(false);

    std::string request_target { m_request.target() };

    http::verb request_method { m_request.method() };

    json request_data;
    if (!m_request.body().empty())
      request_data = json::parse(m_request.body());

    auto [result, response_data] = m_server->handle(request_target,
                                                    request_method,
                                                    request_data);

    switch (result) {
      case http::status::not_found:
        m_response.set(http::field::content_type, "text/plain");
        ostream(m_response.body())
          << "File not found";
        break;
      case http::status::bad_request:
        m_response.set(http::field::content_type, "text/plain");
        ostream(m_response.body())
          << "Invalid request method '" << std::string(m_request.method_string()) << "'";
        break;
      case http::status::ok:
        m_response.set(http::field::content_type, "application/json");
        ostream(m_response.body()) << response_data;
        break;
    }

    m_response.result(result);
    m_response.content_length(m_response.body().size());

    write_response();
  }

  void write_response()
  {
    auto this_ { shared_from_this() };

    http::async_write(
      m_socket,
      m_response,
      [this_](error_code ec, std::size_t)
      {
        this_->m_socket.shutdown(ip::tcp::socket::shutdown_send, ec);
        this_->m_timer.cancel();
      });
  }

  void handle_timeout()
  {
    auto this_ { shared_from_this() };

    m_timer.async_wait(
      [this_](error_code ec)
      {
        if (!ec)
          this_->m_socket.close(ec);
      });
  }

  HTTPServer const *m_server;

  ip::tcp::socket m_socket;

  flat_buffer m_buffer { BUFFER_SIZE };
  steady_timer m_timer;

  http::request<http::string_body> m_request;
  http::response<http::dynamic_body> m_response;
};

std::shared_ptr<HTTPConnection> make_connection(HTTPServer const *server,
                                                ip::tcp::socket &&socket)
{
  return std::make_shared<HTTPConnection>(server, std::move(socket));
}

void accept_connections(HTTPServer const *server,
                        ip::tcp::acceptor &acceptor,
                        ip::tcp::socket &socket)
{
  acceptor.async_accept(
    socket,
    [server, &acceptor, &socket](error_code ec)
    {
      if (ec)
        std::cerr << ec.message() << std::endl;
      else
        make_connection(server, std::move(socket))->run();

      accept_connections(server, acceptor, socket);
    });
}

HTTPServer::HTTPServer(std::string const &addr, uint16_t port)
: m_addr { addr },
  m_port { port }
{}

void HTTPServer::support(std::string const &target,
                         method const &method,
                         handler handler)
{
  std::scoped_lock lock(m_handlers_mtx);

  m_handlers["/" + target].emplace_back(method, std::move(handler));
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
  io_context context { 1 };

  ip::tcp::acceptor acceptor { context, { ip::make_address(m_addr), m_port } };
  ip::tcp::socket socket { context };

  accept_connections(this, acceptor, socket);

  context.run();
}

} // end namespace bm
