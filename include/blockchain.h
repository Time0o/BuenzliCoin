#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "clock.h"
#include "crypto/hash.h"
#include "difficulty.h"
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
    m_nonce { 0 },
    m_index { last ? last->m_index + 1 : 0 },
    m_hash_prev { last ? std::optional<digest> { last->m_hash } : std::nullopt },
    m_hash { determine_hash() }
  {}

  std::string data() const
  { return m_data; }

  clock::TimePoint timestamp() const
  { return m_timestamp; }

  uint64_t index() const
  { return m_index; }

  digest hash() const
  { return m_hash; }

  bool valid() const
  {
    return (m_hash == determine_hash()) &&
           (m_timestamp - config().block_gen_time_max_delta < clock::now());
  }

  bool is_genesis() const
  { return m_index == 0 && !m_hash_prev; }

  bool is_successor_of(Block const &prev) const
  {
    return (m_index == prev.m_index + 1) &&
           (m_hash_prev && *m_hash_prev == prev.m_hash) &&
           (prev.m_timestamp - config().block_gen_time_max_delta < m_timestamp);
  }

  double max_difficulty() const
  { return std::pow(2.0, m_hash.leading_zeros()); }

#ifdef PROOF_OF_WORK
  void adjust_difficulty(double difficulty)
  {
    auto difficulty_log2 { static_cast<std::size_t>(std::log2(difficulty)) };

    assert(difficulty_log2 <= digest::length() * 8);

    for (;;) {
      m_timestamp = clock::now();

      auto maybe_hash { determine_hash() };
      if (maybe_hash.leading_zeros() >= difficulty_log2) {
        m_hash = maybe_hash;
        break;
      }

      ++m_nonce;
    }
  }
#endif // PROOF_OF_WORK

  json to_json() const
  {
    json j;

    j["data"] = m_data;
    j["timestamp"] = clock::to_time_since_epoch(m_timestamp);
    j["nonce"] = m_nonce;
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
    auto nonce { j["nonce"].get<std::size_t>() };
    auto index { j["index"].get<uint64_t>() };

    auto hash { digest::from_string(j["hash"].get<std::string>()) };

    std::optional<digest> hash_prev;
    if (j.count("hash_prev"))
        hash_prev = digest::from_string(j["hash_prev"].get<std::string>());

    return Block {
      data,
      timestamp,
      nonce,
      index,
      hash,
      hash_prev
    };
  }

private:
  Block(std::string const &data,
        clock::TimePoint const &timestamp,
        std::size_t nonce,
        uint64_t index,
        digest const &hash,
        std::optional<digest> const &hash_prev)
  : m_data(data),
    m_timestamp(timestamp),
    m_nonce(nonce),
    m_index(index),
    m_hash(hash),
    m_hash_prev(hash_prev)
  {}

  digest determine_hash() const
  {
    std::stringstream ss;

    ss << m_data;
    ss << clock::to_time_since_epoch(m_timestamp);
    ss << m_nonce;
    ss << m_index;

    if (m_hash_prev)
      ss << m_hash_prev->to_string();

    return HASHER::instance().hash(ss.str());
  }

  std::string m_data;
  clock::TimePoint m_timestamp;
  std::size_t m_nonce;
  uint64_t m_index;

  std::optional<digest> m_hash_prev;
  digest m_hash;
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
#ifdef PROOF_OF_WORK
  , m_difficulty_adjuster { std::move(other.m_difficulty_adjuster) }
#endif // PROOF_OF_WORK
  {}

  void operator=(Blockchain &&other)
  {
    std::scoped_lock lock { m_mtx, other.m_mtx };

    m_blocks = std::move(other.m_blocks);
#ifdef PROOF_OF_WORK
    m_difficulty_adjuster = std::move(other.m_difficulty_adjuster);
#endif // PROOF_OF_WORK
  }

  auto operator<=>(Blockchain const &other) const
  {
    std::scoped_lock lock { m_mtx };

#ifdef PROOF_OF_WORK
    return cumulative_difficulty() <=> other.cumulative_difficulty();
#else
    return length() <=> other.length();
#endif // PROOF_OF_WORK
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

#ifdef PROOF_OF_WORK
  std::size_t cumulative_difficulty() const
  {
    std::scoped_lock lock { m_mtx };

    return m_difficulty_adjuster.cumulative_difficulty();
  }
#endif // PROOF_OF_WORK

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

  void construct_next_block(std::string const &data)
  {
    std::scoped_lock lock { m_mtx };

    std::optional<Block<HASHER>> last_block;

    if (!m_blocks.empty())
      last_block = m_blocks.back();

    Block<HASHER> block { data, last_block };

#ifdef PROOF_OF_WORK
    m_difficulty_adjuster.adjust(block.timestamp());

    block.adjust_difficulty(m_difficulty_adjuster.difficulty());
#endif // PROOF_OF_WORK

    m_blocks.emplace_back(std::move(block));
  }

  void append_next_block(Block<HASHER> block)
  {
    std::scoped_lock lock { m_mtx };

    if (m_blocks.empty()) {
      if (!valid_genesis_block(block))
        throw std::logic_error("attempted appending invalid genesis block");

    } else {
      if (!valid_next_block(block, m_blocks.back()))
        throw std::logic_error("attempted appending invalid next block");
    }

#ifdef PROOF_OF_WORK
    m_difficulty_adjuster.adjust(block.timestamp());

    if (block.max_difficulty() < m_difficulty_adjuster.difficulty())
      throw std::logic_error("attempted appending a block with invalid difficulty");
#endif // PROOF_OF_WORK

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
    Blockchain bchain;

    for (auto const &j_block : j)
      bchain.append_next_block(Block<HASHER>::from_json(j_block));

    return std::move(bchain);
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

#ifdef PROOF_OF_WORK
  DifficultyAdjuster m_difficulty_adjuster;
#endif // PROOF_OF_WORK

  mutable std::recursive_mutex m_mtx;
};

} // end namespace bc
