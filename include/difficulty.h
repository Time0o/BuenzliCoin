#pragma once

#include <cstdint>

#include "clock.h"

namespace bc
{

class DifficultyAdjuster
{
public:
  DifficultyAdjuster(clock::TimeInterval target,
                     std::size_t difficulty_init,
                     std::size_t difficulty_adjust_after,
                     double difficulty_adjust_factor_limit)
  : m_target { target },
    m_difficulty { difficulty_init },
    m_difficulty_adjust_after { difficulty_adjust_after },
    m_difficulty_adjust_factor_limit { difficulty_adjust_factor_limit }
  {}

  std::size_t difficulty() const
  { return m_difficulty; }

  void adjust(clock::TimePoint timestamp)
  {
    if (m_counter == 0) {
      m_difficulty_adjust_timestamp = timestamp;

    } else if (m_counter % m_difficulty_adjust_after == 0) {
      // Time that should have ideally elapsed between the last difficulty
      // adjustment and the generation of the most current block.
      auto gen_time_ref { m_target * m_difficulty_adjust_after };

      // Time that has actually elapsed.
      auto gen_time { timestamp - m_difficulty_adjust_timestamp };

      auto adjust_factor { static_cast<double>(gen_time_ref.count()) /
                           static_cast<double>(gen_time.count()) };

      if (adjust_factor < 1.0 / m_difficulty_adjust_factor_limit)
        adjust_factor = 1.0 / m_difficulty_adjust_factor_limit;
      else if (adjust_factor > m_difficulty_adjust_factor_limit)
        adjust_factor = m_difficulty_adjust_factor_limit;

      m_difficulty *= adjust_factor;

      m_difficulty_adjust_timestamp = timestamp;
    }

    ++m_counter;
  }

private:
  // Time it should take to generate a single block.
  clock::TimeInterval m_target;

  // Current difficulty.
  std::size_t m_difficulty;;
  // Number of blocks after which the difficulty should be readjusted.
  std::size_t m_difficulty_adjust_after;
  // Time point at which the difficulty was last adjusted.
  clock::TimePoint m_difficulty_adjust_timestamp;
  // Limits the maximum possible difficulty adjustment.
  double m_difficulty_adjust_factor_limit;

  std::size_t m_counter { 0 };
};

} // end namespace bc
