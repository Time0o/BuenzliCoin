#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <chrono>
#include <cstdint>

#include "clock.h"
#include "difficulty.h"

using namespace std::literals::chrono_literals;

using namespace bc;

TEST_CASE("difficulty_test", "[difficulty]")
{
  static constexpr clock::TimeInterval TARGET { 10 };
  static constexpr std::size_t DIFFICULTY_INIT { 1 };
  static constexpr std::size_t DIFFICULTY_ADJUST_AFTER { 10 };
  static constexpr double DIFFICULTY_ADJUST_FACTOR_LIMIT { 2.0 };

  DifficultyAdjuster da { TARGET,
                          DIFFICULTY_INIT,
                          DIFFICULTY_ADJUST_AFTER,
                          DIFFICULTY_ADJUST_FACTOR_LIMIT };

  clock::TimePoint t {};

  da.adjust(t);

  REQUIRE(da.difficulty() == DIFFICULTY_INIT);

  SECTION("perfect timing")
  {
    for (int i { 0 }; i < 100 * DIFFICULTY_ADJUST_AFTER; ++i) {
      t += TARGET;

      da.adjust(t);

      INFO("Block " << i + 2);
      CHECK(da.difficulty() == DIFFICULTY_INIT);
    }
  }

  SECTION("difficulty adjusted")
  {
    // Upward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += TARGET / 2;

      da.adjust(t);
    }

    CHECK(da.difficulty() == 2 * DIFFICULTY_INIT);

    // Limited upward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += TARGET / 10;

      da.adjust(t);
    }

    CHECK(da.difficulty() == 2 * DIFFICULTY_ADJUST_FACTOR_LIMIT * DIFFICULTY_INIT);

    // Downward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += 2 * TARGET;

      da.adjust(t);
    }

    CHECK(da.difficulty() == DIFFICULTY_ADJUST_FACTOR_LIMIT * DIFFICULTY_INIT);

    // Limited downward adjustment.
    for (int i { 0 }; i < DIFFICULTY_ADJUST_AFTER; ++i) {
      t += 10 * TARGET;

      da.adjust(t);
    }

    CHECK(da.difficulty() == DIFFICULTY_INIT);
  }
}
