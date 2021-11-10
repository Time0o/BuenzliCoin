#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace bm
{

class HTTPConnection
{
  enum { READ_BUFFER_SIZE = 8192 };

public:
  HTTPConnection(boost::asio::ip::tcp::socket&& socket)
  : m_socket(std::move(socket))
  {}

  void run()
  {
    boost::beast::http::async_read(
      m_socket,
      m_read_buffer,
      m_request,
      [](boost::beast::error_code ec, std::size_t)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          std::cout << "received request" << std::endl; // XXX
      });
  }

private:
  boost::asio::ip::tcp::socket m_socket;
  boost::beast::flat_buffer m_read_buffer { READ_BUFFER_SIZE };
  boost::beast::http::request<boost::beast::http::dynamic_body> m_request;
};

class HTTPServer
{
public:
  HTTPServer(std::string const &addr, uint16_t port)
  : m_addr { boost::asio::ip::make_address(addr) },
    m_port { port }
  {}

  void run()
  {
    boost::asio::io_context context { 1 };

    boost::asio::ip::tcp::acceptor acceptor { context, { m_addr, m_port } };
    boost::asio::ip::tcp::socket socket { context };

    accept_connections(acceptor, socket);

    context.run();
  }

private:
  static std::shared_ptr<HTTPConnection> make_connection(
    boost::asio::ip::tcp::socket &socket)
  {
    return std::make_shared<HTTPConnection>(std::move(socket));
  }

  static void accept_connections(
    boost::asio::ip::tcp::acceptor &acceptor,
    boost::asio::ip::tcp::socket &socket)
  {
    acceptor.async_accept(
      socket,
      [&](boost::beast::error_code ec)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          make_connection(socket)->run();

        accept_connections(acceptor, socket);
      });
  }

  boost::asio::ip::address m_addr;
  uint16_t m_port;
};

} // end namespace bm
