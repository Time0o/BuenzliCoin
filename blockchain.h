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

  static Block from_json(web::json::value j)
  {
    auto data { j["data"].as_string() };
    auto timestamp { timestamp_from_string(j["timestamp"].as_string()) };

    auto index { j["index"].as_number().to_uint64() };

    auto hash { j["hash"] };

    std::optional<typename HASHER::digest> hash_prev;
    if (j.has_field("hash_prev"))
        hash_prev = hash_from_string(j["hash_prev"]);

    return Block { data, timestamp, index, hash, hash_prev };
  }

private:
  explicit Block(std::string const &data,
                 Clock::time_point const &timestamp,
                 uint64_t index,
                 HASHER::digest const &hash,
                 std::optional<typename HASHER::digest> const &hash_prev)
  : m_data(data),
    m_timestamp(timestamp),
    m_index(index),
    m_hash(hash),
    m_hash_prev(hash_prev)
  {}

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

  static Clock::time_point timestamp_from_string(std::string const &str)
  {
    std::tm tm {};
    std::get_time(&tm, "%Y-%m-%d %X");

    return Clock::from_time_t(std::mktime(&tm));
  }

  static std::string hash_to_string(HASHER::digest const &hash)
  {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (auto const &byte : hash)
      ss << std::setw(2) << static_cast<int>(byte);

    return ss.str();
  }

  static HASHER::digest hash_from_string(std::string const &str)
  {
    if (str.size() != HASHER::digest::size() * 2)
      throw std::invalid_argument("invalid hash string");

    auto char_to_nibble = [](char c){
      if (c >= '0' && c <= '9')
        return c - '0';

      if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

      if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

      throw std::invalid_argument("invalid hash string");
    };

    typename HASHER::digest d;
    for (std::size_t i = 0; i < str.size(); ++i)
      d[i] = char_to_nibble(str[i]) << 4 | char_to_nibble(str[i + 1]);

    return d;
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
  Blockchain() = default;

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

  static Blockchain from_json(web::json::value j)
  {
    std::vector<Block<HASHER>> blocks;
    for (auto const &j_block : j.as_array())
      blocks.push_back(Block<HASHER>::from_json(j_block));

    return Blockchain { blocks };
  }

private:
  explicit Blockchain(std::vector<Block<HASHER>> const &blocks)
  : m_blocks(blocks)
  {}

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
