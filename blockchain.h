#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "hash.h"

namespace bm
{

using Clock = std::chrono::high_resolution_clock;

template<typename T, typename HASHER = SHA256Hasher>
class Block
{

public:
  using timestamp_t = Clock::time_point;
  using hash_t = std::array<std::byte, 32>;

  explicit Block(T const &data, Block const *last = nullptr)
  : m_data(data),
    m_timestamp(Clock::now()),
    m_index(last ? last->m_index + 1 : 0),
    m_prev_hash(last ? last->m_hash : std::nullopt),
    m_hash(hash())
  {}

  hash_t hash() const
  {
    std::stringstream ss;

    ss << m_data << m_timestamp << m_index;

    if (m_prev_hash) {
      for (auto prev_hash_byte : *m_prev_hash)
        ss << std::to_integer<int>(prev_hash_byte);
    }

    return HASHER::instance().hash(ss.str());
  }

private:

  T m_data;
  timestamp_t m_timestamp;

  std::size_t m_index;

  std::optional<hash_t> m_prev_hash;
  hash_t m_hash;
};

template<typename T, typename HASHER = SHA256Hasher>
class Blockchain
{
public:
  auto operator<=>(Blockchain const &other)
  {
    return m_blocks.size() <=> other.m_blocks.size();
  }

  void append(Block<T, HASHER> const &block)
  {
    if (m_blocks.empty()) {
      if (!valid_genesis_block(block))
        throw std::logic_error("attempted appending invalid genesis block");

    } else {
      if (!valid_next_block(block, m_blocks.back()))
        throw std::logic_error("attempted appending invalid next block");
    }

    m_blocks.push_back(block);
  }

  bool valid() const
  {
    if (m_blocks.empty())
      return false;

    if (!valid_genesis_block(m_blocks[0]))
      return false;

    for (std::size_t i = 1; i < m_blocks.size(); ++i) {
      if (!valid_next_block(m_blocks[i], m_blocks[i - 1]))
        return false;
    }

    return true;
  }

private:
  static bool valid_genesis_block(Block<T, HASHER> const &block)
  {
    return block.m_index == 0 &&
           !block.m_prev_hash &&
           block.m_hash = block.hash();
  }

  static bool valid_next_block(Block<T, HASHER> const &block,
                               Block<T, HASHER> const &prev_block)
  {
    if (block.m_index != prev_block.m_index + 1)
      return false;

    if (!block.m_prev_hash || (*block.m_prev_hash != prev_block.m_hash))
      return false;

    if (block.m_hash != block.hash())
      return false;

    return true;
  }

  std::vector<Block<T, HASHER>> m_blocks;
};

} // end namespace bm
