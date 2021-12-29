#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <boost/log/trivial.hpp>

#include "format.h"

namespace bc::log
{

enum LogLevel
{
  DEBUG,
  INFO,
  WARNING,
  ERROR
};

void init(LogLevel level);

class Logger
{
public:
  Logger() = default;

  template<typename ...ARGS>
  Logger(char const *prefix_fmt, ARGS &&...args)
  : m_prefix { fmt::format(prefix_fmt, std::forward<ARGS>(args)...) }
  {}

  #define LOG(func, level) \
    template<typename ...ARGS> \
    void func(std::string_view msg_fmt, ARGS &&...args) const \
    { log(level, msg_fmt, std::forward<ARGS>(args)...); } \

  LOG(debug, DEBUG)
  LOG(info, INFO)
  LOG(warning, WARNING)
  LOG(error, ERROR)

private:
  template<typename ...ARGS>
  void log(LogLevel level, std::string_view msg_fmt, ARGS &&...args) const
  {
    std::stringstream ss;

    if (!m_prefix.empty())
      ss << m_prefix << ' ';

    if (sizeof...(args) == 0)
      ss << msg_fmt;
    else
      ss << fmt::format(msg_fmt, std::forward<ARGS>(args)...);

    log(level, ss.str());
  }

  void log(LogLevel level, std::string const &msg) const;

  std::string m_prefix;
};

} // end namespace bc::log
