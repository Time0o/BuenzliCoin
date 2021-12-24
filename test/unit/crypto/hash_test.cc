#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <array>
#include <ostream>
#include <string>

#include "crypto/hash.h"

namespace bc {

template<std::size_t DIGEST_LEN>
std::ostream &operator<<(std::ostream &os, Digest<DIGEST_LEN> const &d)
{
  os << d.to_string();
  return os;
}

} // end namespace bc

using namespace bc;

TEST_CASE("hash_test", "[crypto]")
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
    CHECK(Digest<2>::from_string("8000").zero_prefix_length() == 0);
    CHECK(Digest<2>::from_string("4000").zero_prefix_length() == 1);
    CHECK(Digest<2>::from_string("2000").zero_prefix_length() == 2);
    CHECK(Digest<2>::from_string("1000").zero_prefix_length() == 3);
    CHECK(Digest<2>::from_string("0800").zero_prefix_length() == 4);
    CHECK(Digest<2>::from_string("0400").zero_prefix_length() == 5);
    CHECK(Digest<2>::from_string("0200").zero_prefix_length() == 6);
    CHECK(Digest<2>::from_string("0100").zero_prefix_length() == 7);
    CHECK(Digest<2>::from_string("0080").zero_prefix_length() == 8);
    CHECK(Digest<2>::from_string("0040").zero_prefix_length() == 9);
    CHECK(Digest<2>::from_string("0020").zero_prefix_length() == 10);
    CHECK(Digest<2>::from_string("0010").zero_prefix_length() == 11);
    CHECK(Digest<2>::from_string("0008").zero_prefix_length() == 12);
    CHECK(Digest<2>::from_string("0004").zero_prefix_length() == 13);
    CHECK(Digest<2>::from_string("0002").zero_prefix_length() == 14);
    CHECK(Digest<2>::from_string("0001").zero_prefix_length() == 15);
    CHECK(Digest<2>::from_string("0000").zero_prefix_length() == 16);
  }
}
