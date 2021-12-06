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

  SECTION("digest difficulty")
  {
    CHECK(Digest<2>::from_string("8000").difficulty() == 0);
    CHECK(Digest<2>::from_string("4000").difficulty() == 1);
    CHECK(Digest<2>::from_string("2000").difficulty() == 2);
    CHECK(Digest<2>::from_string("1000").difficulty() == 3);
    CHECK(Digest<2>::from_string("0800").difficulty() == 4);
    CHECK(Digest<2>::from_string("0400").difficulty() == 5);
    CHECK(Digest<2>::from_string("0200").difficulty() == 6);
    CHECK(Digest<2>::from_string("0100").difficulty() == 7);
    CHECK(Digest<2>::from_string("0080").difficulty() == 8);
    CHECK(Digest<2>::from_string("0040").difficulty() == 9);
    CHECK(Digest<2>::from_string("0020").difficulty() == 10);
    CHECK(Digest<2>::from_string("0010").difficulty() == 11);
    CHECK(Digest<2>::from_string("0008").difficulty() == 12);
    CHECK(Digest<2>::from_string("0004").difficulty() == 13);
    CHECK(Digest<2>::from_string("0002").difficulty() == 14);
    CHECK(Digest<2>::from_string("0001").difficulty() == 15);
    CHECK(Digest<2>::from_string("0000").difficulty() == 16);
  }
}
