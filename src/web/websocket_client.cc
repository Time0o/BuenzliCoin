#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "json.h"
#include "web/websocket_client.h"

using namespace boost::asio;
using namespace boost::beast;

namespace bm
{

struct WebSocketClient::Context
{
  Context(std::string const &host, uint16_t port)
  {
    ip::tcp::resolver resolver { ioc };
    server = resolver.resolve(host, std::to_string(port));
    server_url = host + ":" + std::to_string(port);
  }

  io_context ioc { 1 };

  ip::tcp::resolver::results_type server;
  std::string server_url;
};

class WebSocketClient::Connection
  : public std::enable_shared_from_this<Connection>
{
public:
  Connection(WebSocketClient::Context &context)
  : m_stream { make_strand(context.ioc) }
  {
    get_lowest_layer(m_stream).connect(context.server);

    get_lowest_layer(m_stream).expires_never();

    m_stream.set_option(
      websocket::stream_base::timeout::suggested(role_type::client));

    m_stream.set_option(websocket::stream_base::decorator(
      [](websocket::request_type &request)
      {
        request.set(http::field::user_agent, USER_AGENT);
      }));

    m_stream.handshake(context.server_url, "/");
  }

  ~Connection()
  {
    m_stream.close(websocket::close_code::normal);
  }

  void send(json request, callback cb)
  {
    auto this_ { shared_from_this() };

    m_send_queue.emplace(std::move(request), std::move(cb));

    if (m_send_queue.size() > 1)
      return;

    auto const &[request_, cb_] = m_send_queue.front();

    m_stream.async_write(
      buffer(request_.dump()),
      [this_, cb_ = std::move(cb_)](error_code ec, std::size_t)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->on_write(std::move(cb_));
      });
  }

private:
  void on_write(callback cb)
  {
    auto this_ { shared_from_this() };

    m_send_queue.pop();

    m_stream.async_read(
      m_buffer,
      [this_, cb = std::move(cb)](error_code ec, std::size_t)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->on_read(std::move(cb));
      });
  }

  void on_read(callback cb)
  {
    auto this_ { shared_from_this() };

    auto buffer_data { buffers_to_string(m_buffer.data()) };
    m_buffer.consume(m_buffer.size());

    bool status;
    std::string answer;

    try {
      json response = json::parse(buffer_data);

      auto status_ { response["status"].get<std::string>() };

      if (status_ == "ok") {
        status = true;
        answer = response["data"].dump();
      } else {
        status = false;
        answer = response["data"].get<std::string>();
      }

    } catch (...) {
      status = false;
      answer = "Malformed response + '" + buffer_data + "'";
    }

    cb(status, answer);

    // XXX Allow multiple writes to be "in flight" concurrently?
    if (!m_send_queue.empty()) {
      auto const &[request_, cb_] = m_send_queue.front();

      m_stream.async_write(
        buffer(request_.dump()),
        [this_, cb_ = std::move(cb_)](error_code ec, std::size_t)
        {
          if (ec)
            std::cerr << ec.message() << std::endl;
          else
            this_->on_write(std::move(cb_));
        });
    }
  }

private:
  std::queue<std::pair<json, callback>> m_send_queue;

  websocket::stream<tcp_stream> m_stream;

  flat_buffer m_buffer;
};

WebSocketClient::WebSocketClient(std::string const &host, uint16_t port)
: m_host { host },
  m_port { port },
  m_context { std::make_unique<Context>(host, port) },
  m_connection { std::make_shared<Connection>(*m_context) }
{}

WebSocketClient::~WebSocketClient()
{}

std::pair<bool, std::string> WebSocketClient::send_sync(json const &request) const
{
  std::promise<void> done_promise;
  std::future<void> done = done_promise.get_future();

  bool success;
  std::string answer;

  send_async(
    request,
    [&](bool success_, std::string const &answer_)
    {
      success = success_;
      answer = answer_;

      done_promise.set_value();
    });

  run();

  // XXX Handle timeout.
  done.wait();

  return { success, answer };
}

void WebSocketClient::send_async(json const &request, callback cb) const
{
  m_connection->send(request, std::move(cb));
}

void WebSocketClient::run() const
{
  m_context->ioc.run();
  m_context->ioc.restart();
}

} // end namespace bm
