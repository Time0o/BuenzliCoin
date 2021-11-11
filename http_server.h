#pragma once

#include <boost/asio.hpp>

#include <string>

namespace bm
{

class HTTPServer
{
public:
  HTTPServer(std::string const &addr, uint16_t port);

  void run();

private:
  boost::asio::ip::address m_addr;
  uint16_t m_port;
};

} // end namespace bm
