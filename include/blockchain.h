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
#include "format.h"
#include "json.h"

namespace bc
{

template<typename T, typename HASHER = SHA256Hasher>
class Block
{
  template<typename T_, typename HASHER_>
  friend class Blockchain;

public:
  using data_type = T;
  using digest = HASHER::digest;

  explicit Block(T data)
  : m_data { std::move(data) },
    m_timestamp { clock::now() },
    m_nonce { 0 },
    m_index { 0 },
    m_hash { determine_hash() }
  {}

  explicit Block(T data, Block const &last)
  : m_data { std::move(data) },
    m_timestamp { clock::now() },
    m_nonce { 0 },
    m_index { last.m_index + 1 },
    m_hash_prev { last.m_hash },
    m_hash { determine_hash() }
  {}

  T const &data() const
  { return m_data; }

  clock::TimePoint timestamp() const
  { return m_timestamp; }

  uint64_t index() const
  { return m_index; }

  digest hash() const
  { return m_hash; }

  std::pair<bool, std::string> valid() const
  {
    auto [data_valid, data_error] = m_data.valid(m_index);

    if (!data_valid)
        return { false, fmt::format("invalid data: {}", data_error) };

    if (m_timestamp - config().block_gen_time_max_delta >= clock::now())
        return { false, "invalid timestamp" };

    if (m_hash != determine_hash())
        return { false, "invalid hash" };

    return { true, "" };
  }

  bool is_genesis() const
  { return m_index == 0 && !m_hash_prev; }

  bool is_successor_of(Block const &prev) const
  {
    return (m_timestamp > prev.m_timestamp - config().block_gen_time_max_delta) &&
           (m_index == prev.m_index + 1) &&
           (m_hash_prev && *m_hash_prev == prev.m_hash);
  }

  double max_difficulty() const
  { return std::pow(2.0, m_hash.zero_prefix_length()); }

#ifdef PROOF_OF_WORK
  void adjust_difficulty(double difficulty)
  {
    auto difficulty_log2 { static_cast<std::size_t>(std::log2(difficulty)) };

    assert(difficulty_log2 <= digest::max_length() * 8);

    for (;;) {
      m_timestamp = clock::now();

      auto maybe_hash { determine_hash() };
      if (maybe_hash.zero_prefix_length() >= difficulty_log2) {
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

    j["data"] = m_data.to_json();
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
    auto data { T::from_json(j["data"]) };
    auto timestamp { clock::from_time_since_epoch(j["timestamp"].get<uint64_t>()) };
    auto nonce { j["nonce"].get<std::size_t>() };
    auto index { j["index"].get<uint64_t>() };

    auto hash { digest::from_string(j["hash"].get<std::string>()) };

    std::optional<digest> hash_prev;
    if (j.count("hash_prev"))
        hash_prev = digest::from_string(j["hash_prev"].get<std::string>());

    return Block {
      std::move(data),
      timestamp,
      nonce,
      index,
      std::move(hash),
      std::move(hash_prev)
    };
  }

private:
  Block(T data,
        clock::TimePoint timestamp,
        std::size_t nonce,
        uint64_t index,
        digest hash,
        std::optional<digest> hash_prev)
  : m_data(std::move(data)),
    m_timestamp(timestamp),
    m_nonce(nonce),
    m_index(index),
    m_hash(std::move(hash)),
    m_hash_prev(std::move(hash_prev))
  {}

  digest determine_hash() const
  {
    std::stringstream ss;

    ss << m_data.to_json();
    ss << clock::to_time_since_epoch(m_timestamp);
    ss << m_nonce;
    ss << m_index;

    if (m_hash_prev)
      ss << m_hash_prev->to_string();

    return HASHER::instance().hash(ss.str());
  }

  T m_data;
  clock::TimePoint m_timestamp;
  std::size_t m_nonce;
  uint64_t m_index;

  std::optional<digest> m_hash_prev;
  digest m_hash;
};

template<typename T, typename HASHER = SHA256Hasher>
class Blockchain
{
public:
  using value_type = Block<T, HASHER>;
  using const_iterator = typename std::vector<value_type>::const_iterator;

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

  std::pair<bool, std::string> valid() const
  {
    std::scoped_lock lock { m_mtx };

    if (m_blocks.empty())
      return { false, "empty blockchain" };

    for (std::size_t i { 0 }; i < m_blocks.size(); ++i) {
      auto const &block { m_blocks[i] };

      auto [block_valid, block_error] = block.valid();

      if (!block_valid)
        return { false, fmt::format("block {}: {}", i, block_error) };
    }

    if (!m_blocks[0].is_genesis())
      return { false, "invalid genesis block" };

    for (uint64_t i = 1; i < m_blocks.size(); ++i) {
      if (!m_blocks[i].is_successor_of(m_blocks[i - 1]))
        return { false, fmt::format("block {}: not a valid successor", i) };
    }

    return { true, "" };
  }

#ifdef PROOF_OF_WORK
  std::size_t cumulative_difficulty() const
  {
    std::scoped_lock lock { m_mtx };

    return m_difficulty_adjuster.cumulative_difficulty();
  }
#endif // PROOF_OF_WORK

  std::vector<value_type> all_blocks() const
  {
    std::scoped_lock lock { m_mtx };

    return m_blocks;
  }

  value_type const &latest_block() const
  {
    std::scoped_lock lock { m_mtx };

    assert(!m_blocks.empty());

    return m_blocks.back();
  }

  void construct_next_block(T data)
  {
    std::scoped_lock lock { m_mtx };

    std::unique_ptr<value_type> block;

    if (m_blocks.empty()) {
      data.link();
      block = std::make_unique<value_type>(std::move(data));
    } else {
      data.link(latest_block().data());
      block = std::make_unique<value_type>(std::move(data), latest_block());
    }

    auto [block_valid, block_error] = block->valid();

    if (!block_valid)
      throw std::logic_error(fmt::format("attempted appending invalid data: {}", block_error));

#ifdef PROOF_OF_WORK
    m_difficulty_adjuster.adjust(block->timestamp());

    block->adjust_difficulty(m_difficulty_adjuster.difficulty());
#endif // PROOF_OF_WORK

    m_blocks.emplace_back(std::move(*block));
  }

  void append_next_block(value_type block)
  {
    std::scoped_lock lock { m_mtx };

    if (m_blocks.empty()) {
      block.m_data.link();

      auto [valid, error] = valid_genesis_block(block);

      if (!valid)
        throw std::logic_error(
          fmt::format("attempted appending invalid genesis block: {}", error));

    } else {
      block.m_data.link(latest_block().data());

      auto [valid, error] = valid_next_block(block, latest_block());

      if (!valid)
        throw std::logic_error(
          fmt::format("attempted appending invalid next block: {}", error));
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
      bchain.append_next_block(value_type::from_json(j_block));

    return std::move(bchain);
  }

private:
  explicit Blockchain(std::vector<value_type> const &blocks)
  : m_blocks(blocks)
  {}

  static std::pair<bool, std::string> valid_genesis_block(
    value_type const &block)
  {
    if (block.index() != 0)
      return { false, "invalid index" };

    if (block.m_hash_prev)
      return { false, "last hash not empty" };

    auto [block_valid, block_error] = block.valid();

    if (!block_valid)
      return { false, block_error };

    return { true, "" };
  }

  static std::pair<bool, std::string> valid_next_block(
    value_type const &block,
    value_type const &block_prev)
  {
    if (block.index() != block_prev.index() + 1)
      return { false, "invalid index" };

    if (!block.m_hash_prev || (*block.m_hash_prev != block_prev.m_hash))
      return { false, "mismatched hashes" };

    auto [block_valid, block_error] = block.valid();

    if (!block_valid)
      return { false, block_error };

    return { true, "" };
  }

  std::vector<value_type> m_blocks;

#ifdef PROOF_OF_WORK
  DifficultyAdjuster m_difficulty_adjuster;
#endif // PROOF_OF_WORK

  mutable std::recursive_mutex m_mtx;
};

} // end namespace bc
