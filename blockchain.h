#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <cpprest/json.h>
#include <fmt/chrono.h>

#include "hash.h"

namespace bm
{

using Clock = std::chrono::high_resolution_clock;

template<typename HASHER = SHA256Hasher>
class Block
{
public:
  explicit Block(std::string const &data,
                 std::optional<Block> const &last = std::nullopt)
  : m_data(data),
    m_timestamp(Clock::now()),
    m_index(last ? last->m_index + 1 : 0),
    m_hash(hash())
  {
    if (last)
      m_hash_prev = last->m_hash;
  }

  bool valid() const
  {
    return m_hash == hash();
  }

  web::json::value to_json() const
  {
    web::json::value j;

    j["data"] = web::json::value(m_data);
    j["timestamp"] = web::json::value(timestamp_to_string(m_timestamp));

    j["index"] = web::json::value(m_index);

    j["hash"] = web::json::value(hash_to_string(m_hash));

    if (m_hash_prev)
      j["hash_prev"] = web::json::value(hash_to_string(*m_hash_prev));

    return j;
  }

private:
  HASHER::digest hash() const
  {
    std::stringstream ss;

    ss << m_data << timestamp_to_string(m_timestamp) << m_index;

    if (m_hash_prev) {
      for (auto byte : *m_hash_prev)
        ss << byte;
    }

    return HASHER::instance().hash(ss.str());
  }

  static std::string timestamp_to_string(Clock::time_point const &timestamp)
  {
    return fmt::format("{:%Y-%m-%d %X}", timestamp);
  }

  static std::string hash_to_string(HASHER::digest const &hash)
  {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (auto const &byte : hash)
      ss << std::setw(2) << static_cast<int>(byte);

    return ss.str();
  }

  std::string m_data;
  Clock::time_point m_timestamp;

  uint64_t m_index;

  HASHER::digest m_hash;
  std::optional<typename HASHER::digest> m_hash_prev;
};

template<typename HASHER = SHA256Hasher>
class Blockchain
{
public:
  auto operator<=>(Blockchain const &other)
  {
    return m_blocks.size() <=> other.m_blocks.size();
  }

  bool valid() const
  {
    if (m_blocks.empty())
      return false;

    if (!valid_genesis_block(m_blocks[0]))
      return false;

    for (uint64_t i = 1; i < m_blocks.size(); ++i) {
      if (!valid_next_block(m_blocks[i], m_blocks[i - 1]))
        return false;
    }

    return true;
  }

  void append(std::string const &data)
  {
    if (m_blocks.empty())
      m_blocks.emplace_back(data);
    else
      m_blocks.emplace_back(data, m_blocks.back());
  }

  void append(Block<HASHER> const &block)
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

  web::json::value to_json() const
  {
    auto j { web::json::value::array(m_blocks.size()) };

    for (uint64_t i = 0; i < m_blocks.size(); ++i)
      j[i] = m_blocks[i].to_json();

    return j;
  }

private:
  static bool valid_genesis_block(Block<HASHER> const &block)
  {
    return block.valid() &&
           block.m_index == 0 &&
           !block.m_hash_prev;
  }

  static bool valid_next_block(Block<HASHER> const &block,
                               Block<HASHER> const &prev_block)
  {
    if (!block.valid())
      return false;

    if (block.m_index != prev_block.m_index + 1)
      return false;

    if (!block.m_hash_prev || (*block.m_hash_prev != prev_block.m_hash))
      return false;

    return true;
  }

  std::vector<Block<HASHER>> m_blocks;
};

} // end namespace bm
