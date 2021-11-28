#include <cassert>
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
#include "web/websocket_error.h"
#include "web/websocket_server.h"

using namespace boost::asio;
using namespace boost::beast;

namespace bm
{

struct WebSocketServer::Context
{
  Context(std::string const &host, uint16_t port)
  : acceptor { ioc, { ip::make_address(host), port } }
  {}

  io_context ioc { 1 };

  ip::tcp::acceptor acceptor;
};

class WebSocketServer::Connection
  : public std::enable_shared_from_this<Connection>
{
public:
  Connection(WebSocketServer const *server, ip::tcp::socket &&socket)
  : m_server { server },
    m_stream { std::move(socket) }
  {}

  static std::shared_ptr<Connection> create(WebSocketServer const *server,
                                            ip::tcp::socket &&socket)
  {
    return std::make_shared<Connection>(server, std::move(socket));
  }

  static void accept(WebSocketServer const *server,
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
      {
        this_->on_run();
      });
  }

private:
  void on_run()
  {
    auto this_ { shared_from_this() };

    m_stream.set_option(
      websocket::stream_base::timeout::suggested(role_type::server));

    m_stream.set_option(websocket::stream_base::decorator(
      [](websocket::response_type& response)
      {
        response.set(http::field::server, SERVER);
      }));

    m_stream.async_accept(
      [this_](error_code ec)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->on_accept();
      });
  }

  void on_accept()
  {
    auto this_ { shared_from_this() };

    m_stream.async_read(
      m_buffer,
      [this_](error_code ec, std::size_t)
      {
        if (ec == websocket::error::closed)
          return;

        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->on_read();
      });
  }

  void on_read()
  {
    auto this_ { shared_from_this() };

    auto buffer_data { buffers_to_string(m_buffer.data()) };
    m_buffer.consume(m_buffer.size());

    json response;

    try {
      json request = json::parse(buffer_data);

      auto [found, answer] = m_server->handle(request["target"], request["data"]);

      if (found) {
        response["status"] = "ok";
        response["data"] = answer;
      } else {
        response["status"] = "not ok";
        response["data"] = "Not found";
      }

    } catch (json::exception const &e) {
      response["status"] = "not ok";
      response["data"] = "Bad request";

    } catch (WebSocketError const &e) {
      response["status"] = "not ok";
      response["data"] = e.what();
    }

    // XXX Close session on failure.

    m_stream.text(true);

    m_stream.async_write(
      buffer(response.dump()),
      [this_](error_code ec, std::size_t)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->on_write();
      });
  }

  void on_write()
  {
    m_buffer.consume(m_buffer.size());

    on_accept();
  }

  WebSocketServer const *m_server;

  websocket::stream<tcp_stream> m_stream;

  flat_buffer m_buffer;
};

WebSocketServer::WebSocketServer(std::string const &host, uint16_t port)
: m_host { host },
  m_port { port },
  m_context { std::make_unique<Context>(host, port) }
{}

WebSocketServer::~WebSocketServer()
{}

void WebSocketServer::support(std::string const &target,
                              WebSocketServer::handler handler)
{
  assert(!target.empty() && target.front() == '/');

  {
    std::scoped_lock lock(m_handlers_mtx);

    m_handlers[target] = std::move(handler);
  }
}

void WebSocketServer::run() const
{
  Connection::accept(this, m_context->ioc, m_context->acceptor);

  m_context->ioc.run();
}

void WebSocketServer::stop() const
{
  m_context->ioc.stop();
}

std::pair<bool, json> WebSocketServer::handle(std::string const &target,
                                              json const &data) const
{
  std::scoped_lock lock(m_handlers_mtx);

  auto it { m_handlers.find(target) };
  if (it == m_handlers.end())
    return { false, {} };

  return { true, it->second(data) };
}

} // end namespace bm
