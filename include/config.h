#pragma once

// XXX Load from file.
namespace bc::config
{

// Interval after which a new block should be mined.
inline constexpr std::chrono::seconds BLOCK_GEN_INTERVAL { 10 };

// Initial block generation difficulty.
inline constexpr std::size_t BLOCK_GEN_DIFFICULTY_INIT { 1 };

// Number of blocks after which the block generation difficulty is adjusted.
inline constexpr std::size_t BLOCK_GEN_DIFFICULTY_ADJUST_AFTER { 10 };

inline constexpr double BLOCK_GEN_DIFFICULTY_ADJUST_FACTOR_LIMIT { 4 };

} // end namespace bc::config
