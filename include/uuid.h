#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace bc::uuid
{

class UUID
{
public:
  UUID()
  : m_uuid { boost::uuids::random_generator()() }
  {}

  bool operator==(UUID const &other) const
  { return m_uuid == other.m_uuid; }

  std::string to_string(bool abbrev = false) const
  {
    auto str { boost::uuids::to_string(m_uuid) };

    return abbrev ? str.substr(0, str.find('-')) : str;
  }

private:
  boost::uuids::uuid m_uuid;
};

} // end namespace bc::uuid
