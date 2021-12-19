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
    CHECK(Digest<2>::from_string("8000").leading_zeros() == 0);
    CHECK(Digest<2>::from_string("4000").leading_zeros() == 1);
    CHECK(Digest<2>::from_string("2000").leading_zeros() == 2);
    CHECK(Digest<2>::from_string("1000").leading_zeros() == 3);
    CHECK(Digest<2>::from_string("0800").leading_zeros() == 4);
    CHECK(Digest<2>::from_string("0400").leading_zeros() == 5);
    CHECK(Digest<2>::from_string("0200").leading_zeros() == 6);
    CHECK(Digest<2>::from_string("0100").leading_zeros() == 7);
    CHECK(Digest<2>::from_string("0080").leading_zeros() == 8);
    CHECK(Digest<2>::from_string("0040").leading_zeros() == 9);
    CHECK(Digest<2>::from_string("0020").leading_zeros() == 10);
    CHECK(Digest<2>::from_string("0010").leading_zeros() == 11);
    CHECK(Digest<2>::from_string("0008").leading_zeros() == 12);
    CHECK(Digest<2>::from_string("0004").leading_zeros() == 13);
    CHECK(Digest<2>::from_string("0002").leading_zeros() == 14);
    CHECK(Digest<2>::from_string("0001").leading_zeros() == 15);
    CHECK(Digest<2>::from_string("0000").leading_zeros() == 16);
  }
}
