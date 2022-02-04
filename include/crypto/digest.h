#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <utility>

namespace bc
{

class Digest
{
public:
  Digest() = default;

  Digest(std::vector<uint8_t> vec)
  : m_vec { std::move(vec) }
  {}

  bool operator==(Digest const &other) const
  { return m_vec == other.m_vec; }

  uint8_t *data()
  { return m_vec.data(); }

  uint8_t const *data() const
  { return m_vec.data(); }

  std::size_t length() const
  { return m_vec.size(); }

  std::size_t zero_prefix_length() const
  {
    std::size_t count { 0 };

    for (std::size_t i { 0 }; i < m_vec.size(); ++i) {
      auto const &byte = m_vec[i];

      uint8_t mask = 0x80;
      for (uint8_t mask { 0x80 }; mask; mask >>= 1) {
        if ((byte & mask) == 0x00)
          ++count;
        else
          return count;
      }
    }

    return count;
  }

  std::string to_string() const
  {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (std::size_t i { 0 }; i < m_vec.size(); ++i)
      ss << std::setw(2) << static_cast<int>(m_vec[i]);

    return ss.str();
  }

  static Digest from_string(std::string const &str)
  {
    auto char_to_nibble = [](char c){
      if (c >= '0' && c <= '9')
        return c - '0';

      if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

      if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

      throw std::invalid_argument("invalid digest string");
    };

    std::vector<uint8_t> vec;
    for (std::size_t i = 0; i < str.length() / 2; ++i)
      vec.push_back((char_to_nibble(str[2 * i]) << 4) | char_to_nibble(str[2 * i + 1]));

    Digest d { vec };

    return d;
  }

private:
  std::vector<uint8_t> m_vec;
};

} // end namespace bc

namespace std
{

template<>
struct hash<bc::Digest>
{
  std::size_t operator()(bc::Digest const &d) const
  {
    return hash<std::string>()(d.to_string());
  }
};

} // end namespace std
