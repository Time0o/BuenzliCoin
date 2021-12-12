#pragma once

#include <string>

#include <toml.hpp>

#include "clock.h"

namespace bc
{

struct Config
{
  // Interval after which a new block should be mined.
  clock::TimeInterval block_gen_interval { 10 };
  // Initial block generation difficulty.
  std::size_t block_gen_difficulty_init { 1 };
  // Number of blocks after which the block generation difficulty is adjusted.
  std::size_t block_gen_difficulty_adjust_after { 10 };
  // Block generation difficulty adjustment limit.
  double block_gen_difficulty_adjust_factor_limit { 4.0 };

  // XXX to_toml

  static Config from_toml(std::string const &filename)
  {
    Config cfg;

    auto t { toml::parse_file(filename) };

    if (t.contains("block_gen") && t["block_gen"].is_table()) {

        auto const &t_block_gen { *t["block_gen"].as_table() };

        // XXX Generalize...

        if (t_block_gen.contains("interval"))
          cfg.block_gen_interval =
            clock::TimeInterval ( *t_block_gen["interval"].value<clock::TimeInterval::rep>() );

        if (t_block_gen.contains("difficulty_init"))
          cfg.block_gen_difficulty_init =
            *t_block_gen["difficulty_init"].value<std::size_t>();

        if (t_block_gen.contains("difficulty_adjust_after"))
          cfg.block_gen_difficulty_adjust_after =
            *t_block_gen["difficulty_adjust_after"].value<std::size_t>();

        if (t_block_gen.contains("difficulty_adjust_factor_limit"))
          cfg.block_gen_difficulty_adjust_factor_limit =
            *t_block_gen["difficulty_adjust_factor_limit"].value<double>();
    }

    return cfg;
  }
};

inline Config& config()
{
  static Config cfg;
  return cfg;
}

} // end namespace bc::config
