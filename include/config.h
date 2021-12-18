#pragma once

#include <string>

#include "clock.h"

namespace bc
{

struct Config
{
  // Interval after which a new block should be mined.
  clock::TimeInterval block_gen_interval { 10000 };
  // Initial block generation difficulty.
  double block_gen_difficulty_init { 2 };
  // Number of blocks after which the block generation difficulty is adjusted.
  std::size_t block_gen_difficulty_adjust_after { 10 };
  // Block generation difficulty adjustment limit.
  double block_gen_difficulty_adjust_factor_limit { 16 };

  // XXX to_toml

  static Config from_toml(std::string const &filename);
};

inline Config& config()
{
  static Config cfg;
  return cfg;
}

} // end namespace bc::config
