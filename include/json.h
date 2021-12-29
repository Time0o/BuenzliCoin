#pragma once

#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

inline json::reference json_get(json &j, std::string const & key)
{
  if (!j.contains(key))
    throw std::invalid_argument("nonexistent key: " + key);

  return j[key];
}

inline json::const_reference json_get(json const &j, std::string const &key)
{
  if (!j.contains(key))
    throw std::invalid_argument("nonexistent key: " + key);

  return j[key];
}
