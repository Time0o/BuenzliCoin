#include <array>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

#include "hash.h"

namespace bc
{

template<typename T>
class Block
{
public:
  using timestamp_t = std::chrono::high_resolution_clock::time_point;
  using hash_t = std::array<uint8_t, 32>;

  explicit Block(T const &data, timestamp_t timestamp, Block const &last)
  : m_data(data),
    m_timestamp(timestamp),
    m_index(last.m_index + 1),
    m_last_hash(last.m_hash),
    m_hash(hash_content())
  {}

private:
  hash_t hash_content() const
  {
    std::stringstream ss;
    ss << m_data << m_timestamp << m_index << m_last_hash;

    return hash::SHA256Hasher::instance().hash(ss.str());
  }

  T m_data;
  timestamp_t m_timestamp;

  std::size_t m_index;
  hash_t m_last_hash;
  hash_t m_hash;
};

} // end namespace bc
