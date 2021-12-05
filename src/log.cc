#include <cassert>
#include <iostream>
#include <string>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include "log.h"

namespace trivial = boost::log::trivial;

namespace bc::log
{

namespace
{

auto to_boost(LogLevel level)
{
  switch (level) {
  case DEBUG:
    return trivial::debug;
  case INFO:
    return trivial::info;
  case WARNING:
    return trivial::warning;
  case ERROR:
    return trivial::error;
  }

  assert(false);
}

} // end namespace

void init(LogLevel level)
{
  boost::log::register_simple_formatter_factory<trivial::severity_level, char>("Severity");

  boost::log::add_console_log(
    std::cout,
    boost::log::keywords::format = R"([%TimeStamp%] [%Severity%] %Message%)");

  boost::log::core::get()->set_filter(trivial::severity >= to_boost(level));

  boost::log::add_common_attributes();
}

void Logger::log(LogLevel level, std::string const &msg) const
{
  switch (level) {
  case DEBUG:
    BOOST_LOG_TRIVIAL(debug) << msg;
    break;
  case INFO:
    BOOST_LOG_TRIVIAL(info) << msg;
    break;
  case WARNING:
    BOOST_LOG_TRIVIAL(warning) << msg;
    break;
  case ERROR:
    BOOST_LOG_TRIVIAL(error) << msg;
    break;
  }
}

} // end namespace bc::log
