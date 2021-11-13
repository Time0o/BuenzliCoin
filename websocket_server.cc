#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "json.h"
#include "websocket_server.h"

using namespace boost::asio;
using namespace boost::beast;

namespace bm
{

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
public:
  WebSocketSession(WebSocketServer const *server, ip::tcp::socket &&socket)
  : m_server { server },
    m_socket { std::move(socket) }
  {}

  void run()
  {
    auto this_ { shared_from_this() };

    dispatch(
      m_socket.get_executor(),
      [this_]{ this_->on_run(); });
  }

private:
  void on_run()
  {
    m_socket.set_option(
      websocket::stream_base::timeout::suggested(role_type::server));

    accept();
  }

  void accept()
  {
    auto this_ { shared_from_this() };

    m_socket.async_accept(
      [this_](error_code ec)
      { this_->on_accept(ec); });
  }

  void on_accept(error_code ec)
  {
    if (ec)
      std::cerr << ec.message() << std::endl;

    read();
  }

  void read()
  {
    auto this_ { shared_from_this() };

    m_socket.async_read(
      m_buffer,
      [this_](error_code ec, std::size_t bytes)
      { this_->on_read(ec, bytes); });
  }

  void on_read(error_code ec, std::size_t)
  {
    if (ec == websocket::error::closed)
      return;

    if (ec)
      std::cerr << ec.message() << std::endl;

    auto buffer_data { buffers_to_string(m_buffer.data()) };
    m_buffer.consume(m_buffer.size());

    json request;
    request = json::parse(buffer_data);

    auto [_, response] { m_server->handle(
      request["target"].get<std::string>(),
      request["data"]) };

    // XXX Close session on failure.

    write(response);
  }

  void write(json const &response)
  {
    auto this_ { shared_from_this() };

    ostream(m_buffer) << response.dump();

    m_socket.text(true);

    m_socket.async_write(
      m_buffer.data(),
      [this_](error_code ec, std::size_t bytes)
      { this_->on_write(ec, bytes); });
  }

  void on_write(error_code ec, std::size_t)
  {
    if (ec)
      std::cerr << ec.message() << std::endl;

    m_buffer.consume(m_buffer.size());

    read();
  }

  WebSocketServer const *m_server;

  websocket::stream<tcp_stream> m_socket;

  flat_buffer m_buffer;
};

std::shared_ptr<WebSocketSession> make_session(WebSocketServer const *server,
                                               ip::tcp::socket &&socket)
{
  return std::make_shared<WebSocketSession>(server, std::move(socket));
}

void accept_connections(WebSocketServer const *server,
                        io_context &context,
                        ip::tcp::acceptor &acceptor)
{
  acceptor.async_accept(
    make_strand(context),
    [server, &context, &acceptor](error_code ec, ip::tcp::socket socket)
    {
      if (ec)
        std::cerr << ec.message() << std::endl;
      else
        make_session(server, std::move(socket))->run();

      accept_connections(server, context, acceptor);
    });
}

WebSocketServer::WebSocketServer(std::string const &addr, uint16_t port)
: m_addr { addr },
  m_port { port }
{}

void WebSocketServer::support(std::string const &target,
                              WebSocketServer::handler handler)
{
  std::scoped_lock lock(m_handlers_mtx);

  m_handlers[target] = std::move(handler);
}

void WebSocketServer::run() const
{
  io_context context { 1 };

  ip::tcp::acceptor acceptor { context, { ip::make_address(m_addr), m_port } };

  accept_connections(this, context, acceptor);

  context.run();
}

std::pair<bool, json> WebSocketServer::handle(std::string const &target,
                                              json const &data) const
{
  std::scoped_lock lock(m_handlers_mtx);

  // XXX Distinguish target not found and bad request.

  auto it { m_handlers.find(target) };
  if (it == m_handlers.end())
    return { false, {} };

  return it->second(data);
}

} // end namespace bm
