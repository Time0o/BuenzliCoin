#pragma once

#include <string>
#include <utility>

#include "json.h"

namespace bc
{

class Text
{
public:
  Text(std::string text)
  : m_text { std::move(text) }
  {}

  std::pair<bool, std::string> valid(std::size_t) const
  { return { true, "" }; }

  json to_json() const
  { return m_text; }

  static Text from_json(json const &j)
  { return Text(j.get<std::string>()); }

private:
  std::string m_text;
};

} // end namespace bc
