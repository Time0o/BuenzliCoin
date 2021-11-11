#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace bm
{

class HTTPConnection : public std::enable_shared_from_this<HTTPConnection>
{
  static constexpr std::size_t BUFFER_SIZE { 8192 };
  static constexpr std::chrono::seconds PROCESS_TIMEOUT { 60 };

public:
  HTTPConnection(boost::asio::ip::tcp::socket&& socket)
  : m_socket { std::move(socket) },
    m_socket_timer { m_socket.get_executor(), PROCESS_TIMEOUT }
  {}

  void run()
  {
    auto this_ { shared_from_this() };

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

    m_socket_timer.async_wait(
      [this_](boost::beast::error_code ec)
      {
        if (ec)
          std::cerr << ec.message() << std::endl;
        else
          this_->m_socket.close(ec);
      });
  }

private:
  boost::asio::ip::tcp::socket m_socket;
  boost::asio::steady_timer m_socket_timer;
  boost::beast::flat_buffer m_read_buffer { BUFFER_SIZE };
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
    boost::asio::ip::tcp::socket &&socket)
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
          make_connection(std::move(socket))->run();

        accept_connections(acceptor, socket);
      });
  }

  boost::asio::ip::address m_addr;
  uint16_t m_port;
};

} // end namespace bm
