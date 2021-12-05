#pragma once

#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

namespace bm::fmt
{

template<typename ...ARGS>
std::string format(std::string_view str_fmt, ARGS &&...args)
{
  return ::fmt::vformat(str_fmt, ::fmt::make_format_args(std::forward<ARGS>(args)...));
}

} // end namespace bm
