#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <string_view>

#include "crypto/keypair.h"

using namespace bc;

std::string_view ec_private_key1 {
  "LYZYhW4AeutWpQ9y5+jEY3YWR1Fohg0fdeEOow4CVVVoAcGBSuBBAAKoUQDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w==" };

std::string_view ec_public_key1 {
  "laLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w==" };

std::string_view ec_private_key2 {
  "MhAttMFB2H70eWRmUrRqxzmr7Q0s6Oi5EzxlBKR/dCfoAcGBSuBBAAKoUQDQgAEzZAc8y92btejhFwuZfUvYNUjWIQUtPyEnHeeLjdtNCZXkN5d/7W2MHVsNZN5fW8CIQdrSWjPJGe//RXvFLakUg==" };

std::string_view ec_public_key2 {
  "zZAc8y92btejhFwuZfUvYNUjWIQUtPyEnHeeLjdtNCZXkN5d/7W2MHVsNZN5fW8CIQdrSWjPJGe//RXvFLakUg==" };

TEST_CASE("keypair_test", "[crypto]")
{
  SECTION("sign and verify")
  {
    ECSecp256k1PrivateKey private_key1(ec_private_key1);
    ECSecp256k1PublicKey public_key1(ec_public_key1);

    ECSecp256k1PrivateKey private_key2(ec_private_key2);
    ECSecp256k1PublicKey public_key2(ec_public_key2);

    auto message { "some message" };

    auto signature1 { private_key1.sign(message) };
    CHECK(public_key1.verify(message, signature1));
    CHECK(!public_key2.verify(message, signature1));

    auto signature2 { private_key2.sign(message) };
    CHECK(!public_key1.verify(message, signature2));
    CHECK(public_key2.verify(message, signature2));
  }
}
