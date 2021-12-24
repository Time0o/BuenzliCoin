#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace bc
{

template<std::size_t DIGEST_LEN>
class Digest
{
public:
  using array = std::array<uint8_t, DIGEST_LEN>;

  Digest()
   : m_length { DIGEST_LEN }
  {}

  Digest(array arr)
  : m_arr { std::move(arr) }
  , m_length { DIGEST_LEN }
  {}

  bool operator==(Digest const &other) const
  { return m_arr == other.m_arr; }

  uint8_t *data()
  { return m_arr.data(); }

  uint8_t const *data() const
  { return m_arr.data(); }

  std::size_t length() const
  { return m_length; }

  static constexpr std::size_t max_length()
  { return DIGEST_LEN; }

  std::size_t zero_prefix_length() const
  {
    std::size_t count { 0 };

    for (std::size_t i { 0 }; i < m_length; ++i) {
      auto const &byte = m_arr[i];

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

  void adjust_length(std::size_t new_length)
  {
    assert(new_length <= m_length);
    m_length = new_length;
  }

  std::string to_string() const
  {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (auto const &byte : m_arr)
      ss << std::setw(2) << static_cast<int>(byte);

    return ss.str();
  }

  static Digest from_string(std::string const &str)
  {
    if (str.size() != std::tuple_size_v<array> * 2)
      throw std::invalid_argument("invalid digest string");

    auto char_to_nibble = [](char c){
      if (c >= '0' && c <= '9')
        return c - '0';

      if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

      if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

      throw std::invalid_argument("invalid digest string");
    };

    array d;
    for (std::size_t i = 0; i < d.size(); ++i)
      d[i] = (char_to_nibble(str[2 * i]) << 4) | char_to_nibble(str[2 * i + 1]);

    return Digest { d };
  }

private:
  array m_arr;
  std::size_t m_length;
};

} // end namespace bc