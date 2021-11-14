#pragma once

#include <stdexcept>
#include <string>

#include "http_server.h"

namespace bm
{

class HTTPError : public std::runtime_error
{
public:
  HTTPError(HTTPServer::status const &status, std::string const &what)
  : std::runtime_error(what),
    m_status(status)
  {}

  HTTPServer::status status() const
  { return m_status; }

private:
  HTTPServer::status m_status;
};

} // end namespace bm
