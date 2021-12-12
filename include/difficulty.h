#pragma once

#include <cmath>
#include <cstdint>

#include "clock.h"
#include "config.h"

namespace bc
{

class DifficultyAdjuster
{
public:
  DifficultyAdjuster()
  : m_difficulty_raw { config().block_gen_difficulty_init }
  , m_difficulty_log2 { static_cast<std::size_t>(std::log2(m_difficulty_raw)) }
  {}

  std::size_t difficulty() const
  { return m_difficulty_log2; }

  void adjust(clock::TimePoint timestamp)
  {
    auto interval { config().block_gen_interval };
    auto adjust_after { config().block_gen_difficulty_adjust_after };
    auto adjust_factor_limit { config().block_gen_difficulty_adjust_factor_limit };

    if (m_counter == 0) {
      m_timestamp = timestamp;

    } else if (m_counter % adjust_after == 0) {

      // Time that should have ideally elapsed between the last difficulty
      // adjustment and the generation of the most current block.
      auto interval_total_expected { interval * adjust_after };

      // Time that has actually elapsed.
      auto interval_total_actual { timestamp - m_timestamp };

      auto adjust_factor {
        static_cast<double>(interval_total_expected.count()) /
        static_cast<double>(interval_total_actual.count()) };

      if (adjust_factor < 1.0 / adjust_factor_limit)
        adjust_factor = 1.0 / adjust_factor_limit;
      else if (adjust_factor > adjust_factor_limit)
        adjust_factor = adjust_factor_limit;

      m_difficulty_raw *= adjust_factor;
      m_difficulty_log2 = static_cast<std::size_t>(std::log2(m_difficulty_raw));

      m_timestamp = timestamp;
    }

    ++m_counter;
  }

private:
  double m_difficulty_raw;
  std::size_t m_difficulty_log2;

  std::size_t m_counter { 0 };

  clock::TimePoint m_timestamp;
};

} // end namespace bc
