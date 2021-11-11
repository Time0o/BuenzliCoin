#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

#include "http_server.h"

using namespace boost::asio;
using namespace boost::beast;
using namespace nlohmann;

namespace {

class HTTPConnection : public std::enable_shared_from_this<HTTPConnection>
{
  static constexpr std::size_t BUFFER_SIZE { 8192 };
  static constexpr std::chrono::seconds PROCESS_TIMEOUT { 60 };

public:
  HTTPConnection(ip::tcp::socket&& socket)
  : m_socket { std::move(socket) },
    m_socket_timer { m_socket.get_executor(), PROCESS_TIMEOUT }
  {}

  void run()
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

    m_socket_timer.async_wait(
      [this_](error_code ec)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->m_socket.close(ec);
      });
  }

private:
  void handle_request()
  {
    m_response.version(m_request.version());
    m_response.keep_alive(false);

    // XXX

    // XXX Handle parse errors.
    json j { m_request.body() };
    std::cout << "received request: " << j << std::endl;

    m_response.result(http::status::ok);
  }

  ip::tcp::socket m_socket;
  steady_timer m_socket_timer;
  flat_buffer m_buffer { BUFFER_SIZE };
  http::request<http::string_body> m_request;
  http::response<http::dynamic_body> m_response;
};

std::shared_ptr<HTTPConnection> make_connection(ip::tcp::socket &&socket)
{
  return std::make_shared<HTTPConnection>(std::move(socket));
}

void accept_connections(ip::tcp::acceptor &acceptor, ip::tcp::socket &socket)
{
  acceptor.async_accept(
    socket,
    [&](error_code ec)
    {
      if (ec)
        std::cerr << ec.message() << std::endl;
      else
        make_connection(std::move(socket))->run();

      accept_connections(acceptor, socket);
    });
}

} // end namespace

namespace bm
{

HTTPServer::HTTPServer(std::string const &addr, uint16_t port)
: m_addr { ip::make_address(addr) },
  m_port { port }
{}

void HTTPServer::run()
{
  io_context context { 1 };

  ip::tcp::acceptor acceptor { context, { m_addr, m_port } };
  ip::tcp::socket socket { context };

  accept_connections(acceptor, socket);

  context.run();
}

} // end namespace bm
