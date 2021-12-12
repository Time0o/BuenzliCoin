#pragma once

#include <chrono>
#include <cstdint>

namespace bc::clock
{

using Clock = std::chrono::system_clock;
using TimeInterval = std::chrono::milliseconds;
using TimePoint = std::chrono::time_point<Clock, TimeInterval>;

inline auto now()
{
  return std::chrono::floor<TimeInterval>(Clock::now());
}

inline uint64_t to_time_since_epoch(TimePoint const &tp)
{
  return tp.time_since_epoch().count();
}

inline TimePoint from_time_since_epoch(uint64_t tse)
{
  return TimePoint { TimeInterval { tse } };
}

} // end namespace bc::clock
