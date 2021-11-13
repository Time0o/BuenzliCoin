#include <cstdint>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "json.h"
#include "websocket_client.h"

using namespace boost::asio;
using namespace boost::beast;

namespace bm
{

struct WebSocketConnection
{
  WebSocketConnection(std::string const &addr, uint16_t port)
  : resolver { context },
    socket { context }
  {
    auto results { resolver.resolve(addr, std::to_string(port)) };

    connect(socket.next_layer(), results.begin(), results.end());

    socket.handshake(addr, "/");
  }

  io_context context;
  ip::tcp::resolver resolver;
  websocket::stream<ip::tcp::socket> socket;
};

WebSocketClient::WebSocketClient(std::string const &addr, uint16_t port)
: m_connection { std::make_unique<WebSocketConnection>(addr, port) }
{}

WebSocketClient::~WebSocketClient()
{
  m_connection->socket.close(websocket::close_code::normal);
}

json WebSocketClient::send(json const &data) const
{
  m_connection->socket.write(buffer(data.dump()));

  flat_buffer buffer; // XXX Should this have static size?
  m_connection->socket.read(buffer);

  return json::parse(buffers_to_string(buffer.data()));
}

} // end namespace bm
