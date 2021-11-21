#pragma once

#include <stdexcept>
#include <string>

namespace bm
{

struct WebSocketError : public std::runtime_error
{
  WebSocketError(std::string const &what)
  : std::runtime_error(what)
  {}
};

} // end namespace bm
