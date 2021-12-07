#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "clock.h"
#include "hash.h"
#include "json.h"

namespace bc
{

template<typename HASHER = SHA256Hasher>
class Block
{
  template<typename HASHER_>
  friend class Blockchain;

  using digest = HASHER::digest;

public:
  explicit Block(std::string const &data,
                 std::optional<Block> const &last = std::nullopt)
  : m_data { data },
    m_timestamp { clock::now() },
    m_index { last ? last->m_index + 1 : 0 },
    m_hash_prev { last ? std::optional<digest> { last->m_hash } : std::nullopt },
    m_hash { hash() }
  {}

  std::string data() const
  { return m_data; }

  clock::TimePoint timestamp() const
  { return m_timestamp; }

  uint64_t index() const
  { return m_index; }

  bool valid() const
  { return m_hash == hash(); }

  bool is_genesis() const
  { return m_index == 0 && !m_hash_prev; }

  bool is_successor_of(Block const &prev) const
  {
    return (m_index == prev.m_index + 1) &&
           (m_hash_prev && *m_hash_prev == prev.m_hash);
  }

  json to_json() const
  {
    json j;

    j["data"] = m_data;
    j["timestamp"] = clock::to_time_since_epoch(m_timestamp);

    j["index"] = m_index;

    j["hash"] = m_hash.to_string();

    if (m_hash_prev)
      j["hash_prev"] = m_hash_prev->to_string();

    return j;
  }

  static Block from_json(json const &j)
  {
    auto data { j["data"].get<std::string>() };
    auto timestamp { clock::from_time_since_epoch(j["timestamp"].get<uint64_t>()) };

    auto index { j["index"].get<uint64_t>() };

    auto hash { digest::from_string(j["hash"].get<std::string>()) };

    std::optional<digest> hash_prev;
    if (j.count("hash_prev"))
        hash_prev = digest::from_string(j["hash_prev"].get<std::string>());

     return Block { data, timestamp, index, hash, hash_prev };
  }

private:
  Block(std::string const &data,
        clock::TimePoint const &timestamp,
        uint64_t index,
        digest const &hash,
        std::optional<digest> const &hash_prev)
  : m_data(data),
    m_timestamp(timestamp),
    m_index(index),
    m_hash(hash),
    m_hash_prev(hash_prev)
  {}

  digest hash() const
  {
    std::stringstream ss;

    ss << m_data;
    ss << clock::to_time_since_epoch(m_timestamp);

    ss << m_index;

    if (m_hash_prev)
      ss << m_hash_prev->to_string();

    return HASHER::instance().hash(ss.str());
  }

  std::string m_data;
  clock::TimePoint m_timestamp;

  uint64_t m_index { 0 };

  digest m_hash;
  std::optional<digest> m_hash_prev;
};

template<typename HASHER = SHA256Hasher>
class Blockchain
{
public:
  using value_type = Block<HASHER>;
  using const_iterator = typename std::vector<Block<HASHER>>::const_iterator;

  Blockchain() = default;

  Blockchain(Blockchain &&other)
  : m_blocks { std::move(other.m_blocks) }
  {}

  void operator=(Blockchain &&other)
  {
    std::scoped_lock lock { m_mtx, other.m_mtx };

    m_blocks = std::move(other.m_blocks);
  }

  std::vector<Block<HASHER>> all_blocks() const
  {
    std::scoped_lock lock { m_mtx };

    return m_blocks;
  }

  Block<HASHER> const &latest_block() const
  {
    std::scoped_lock lock { m_mtx };

    assert(!m_blocks.empty());

    return m_blocks.back();
  }

  bool empty() const
  {
    std::scoped_lock lock { m_mtx };

    return m_blocks.empty();
  }

  std::size_t length() const
  {
    std::scoped_lock lock { m_mtx };

    return m_blocks.size();
  }

  bool valid() const
  {
    std::scoped_lock lock { m_mtx };

    if (m_blocks.empty())
      return false;

    for (auto const &block : m_blocks) {
      if (!block.valid())
        return false;
    }

    if (!m_blocks[0].is_genesis())
      return false;

    for (uint64_t i = 1; i < m_blocks.size(); ++i) {
      if (!m_blocks[i].is_successor_of(m_blocks[i - 1]))
        return false;
    }

    return true;
  }

  void append(std::string const &data)
  {
    std::scoped_lock lock { m_mtx };

    if (m_blocks.empty())
      m_blocks.emplace_back(data);
    else
      m_blocks.emplace_back(data, m_blocks.back());
  }

  void append(Block<HASHER> block)
  {
    std::scoped_lock lock { m_mtx };

    if (m_blocks.empty()) {
      if (!valid_genesis_block(block))
        throw std::logic_error("attempted appending invalid genesis block");

    } else {
      if (!valid_next_block(block, m_blocks.back()))
        throw std::logic_error("attempted appending invalid next block");
    }

    m_blocks.emplace_back(std::move(block));
  }

  json to_json() const
  {
    std::scoped_lock lock { m_mtx };

    json j = json::array();

    for (uint64_t i = 0; i < m_blocks.size(); ++i)
      j.push_back(m_blocks[i].to_json());

    return j;
  }

  static Blockchain from_json(json const &j)
  {
    std::vector<Block<HASHER>> blocks;

    for (auto const &j_block : j)
    {
      auto block { Block<HASHER>::from_json(j_block) };

      blocks.push_back(std::move(block));
    }

    return Blockchain { blocks };
  }

private:
  explicit Blockchain(std::vector<Block<HASHER>> const &blocks)
  : m_blocks(blocks)
  {}

  static bool valid_genesis_block(Block<HASHER> const &block)
  {
    return block.valid() &&
           block.index() == 0 &&
           !block.m_hash_prev;
  }

  static bool valid_next_block(Block<HASHER> const &block,
                               Block<HASHER> const &block_prev)
  {
    if (!block.valid())
      return false;

    if (block.index() != block_prev.index() + 1)
      return false;

    if (!block.m_hash_prev || (*block.m_hash_prev != block_prev.m_hash))
      return false;

    return true;
  }

  std::vector<Block<HASHER>> m_blocks;
  mutable std::mutex m_mtx;
};

} // end namespace bc
