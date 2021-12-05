#pragma once

#include <stdexcept>
#include <string>

namespace bc
{

struct WebSocketError : public std::runtime_error
{
  WebSocketError(std::string const &what)
  : std::runtime_error(what)
  {}
};

} // end namespace bc
