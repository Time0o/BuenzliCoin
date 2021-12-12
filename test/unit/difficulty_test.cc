#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>

#include "clock.h"
#include "config.h"
#include "difficulty.h"

using namespace std::literals::chrono_literals;

using namespace bc;

TEST_CASE("difficulty_test", "[difficulty]")
{
  static constexpr clock::TimeInterval INTERVAL { 10000 };
  static constexpr double DIFFICULTY_INIT { 2 };
  static constexpr std::size_t DIFFICULTY_ADJUST_AFTER { 10 };
  static constexpr double DIFFICULTY_ADJUST_FACTOR_LIMIT { 16 };

  config().block_gen_interval = INTERVAL;
  config().block_gen_difficulty_init = DIFFICULTY_INIT;
  config().block_gen_difficulty_adjust_after = DIFFICULTY_ADJUST_AFTER;
  config().block_gen_difficulty_adjust_factor_limit = DIFFICULTY_ADJUST_FACTOR_LIMIT;

  DifficultyAdjuster da;

  clock::TimePoint t {};

  da.adjust(t);

  REQUIRE(da.difficulty() == std::log2(DIFFICULTY_INIT));

  SECTION("perfect timing")
  {
    for (int i { 0 }; i < 100 * DIFFICULTY_ADJUST_AFTER; ++i) {
      t += INTERVAL;

      da.adjust(t);

      INFO("Block " << i + 2);
      CHECK(da.difficulty() == std::log2(DIFFICULTY_INIT));
    }
  }

  SECTION("difficulty adjusted")
  {
    // Upward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += INTERVAL / 2;

      da.adjust(t);
    }

    CHECK(da.difficulty() == std::log2(2 * DIFFICULTY_INIT));

    // Limited upward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += INTERVAL / static_cast<std::size_t>(2 * DIFFICULTY_ADJUST_FACTOR_LIMIT);

      da.adjust(t);
    }

    CHECK(da.difficulty() == std::log2(2 * DIFFICULTY_ADJUST_FACTOR_LIMIT * DIFFICULTY_INIT));

    // Downward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += 2 * INTERVAL;

      da.adjust(t);
    }

    CHECK(da.difficulty() == std::log2(DIFFICULTY_ADJUST_FACTOR_LIMIT * DIFFICULTY_INIT));

    // Limited downward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += static_cast<std::size_t>(2 * DIFFICULTY_ADJUST_FACTOR_LIMIT) * INTERVAL;

      da.adjust(t);
    }

    CHECK(da.difficulty() == std::log2(DIFFICULTY_INIT));
  }
}
