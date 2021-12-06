#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <array>
#include <ostream>
#include <string>

#include "hash.h"

namespace bc {

template<std::size_t DIGEST_LEN>
std::ostream &operator<<(std::ostream &os, Digest<DIGEST_LEN> const &d)
{
  os << d.to_string();
  return os;
}

} // end namespace bc

using namespace bc;

TEST_CASE("hash_test", "[hash]")
{
  SECTION("digest string conversions")
  {
    Digest<4>::array d_arr { 0xde, 0xad, 0xbe, 0xef };
    std::string d_string { "deadbeef" };

    Digest<4> d { d_arr };

    CHECK(d.to_string() == d_string);
    CHECK(Digest<4>::from_string(d_string) == d);
  }
}
