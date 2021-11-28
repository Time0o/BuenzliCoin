#pragma once

#include <chrono>
#include <cstdint>

namespace bm::clock
{

using Clock = std::chrono::system_clock;
using TimePrecision = std::chrono::milliseconds;
using TimePoint = std::chrono::time_point<Clock, TimePrecision>;

inline auto now()
{
  return std::chrono::floor<TimePrecision>(Clock::now());
}

inline uint64_t to_time_since_epoch(TimePoint const &tp)
{
  return tp.time_since_epoch().count();
}

inline TimePoint from_time_since_epoch(uint64_t tse)
{
  return TimePoint { TimePrecision { tse } };
}

} // end namespace bm::clock
