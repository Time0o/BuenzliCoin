#pragma once

#include <cstdint>

#include "clock.h"
#include "config.h"

namespace bc
{

class DifficultyAdjuster
{
public:
  DifficultyAdjuster()
  : m_difficulty { config().blockgen_difficulty_init }
  {}

  double difficulty() const
  { return m_difficulty; }

  double cumulative_difficulty() const
  { return m_cumulative_difficulty; }

  void adjust(clock::TimePoint timestamp)
  {
    auto time_expected { config().blockgen_time_expected };
    auto adjust_after { config().blockgen_difficulty_adjust_after };
    auto adjust_factor_limit { config().blockgen_difficulty_adjust_factor_limit };

    if (m_counter == 0) {
      m_timestamp = timestamp;

    } else if (m_counter % adjust_after == 0) {

      // Time that should have ideally elapsed between the last difficulty
      // adjustment and the generation of the most current block.
      auto total_time_expected { time_expected * adjust_after };

      // Time that has actually elapsed.
      auto total_time_actual { timestamp - m_timestamp };

      auto adjust_factor {
        static_cast<double>(total_time_expected.count()) /
        static_cast<double>(total_time_actual.count()) };

      if (adjust_factor < 1.0 / adjust_factor_limit)
        adjust_factor = 1.0 / adjust_factor_limit;
      else if (adjust_factor > adjust_factor_limit)
        adjust_factor = adjust_factor_limit;

      m_difficulty *= adjust_factor;
      m_cumulative_difficulty += m_difficulty;

      m_timestamp = timestamp;
    }

    ++m_counter;
  }

private:
  double m_difficulty;
  double m_cumulative_difficulty { 0.0 };

  std::size_t m_counter { 0 };

  clock::TimePoint m_timestamp;
};

} // end namespace bc
