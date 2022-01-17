#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <string_view>

#include "crypto/keypair.h"

using namespace bc;

std::string_view ec_private_key1 {
  "MHQCAQEEILYZYhW4AeutWpQ9y5+jEY3YWR1Fohg0fdeEOow4CVVVoAcGBSuBBAAKoUQDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w" };

std::string_view ec_public_key1 {
  "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w" };

std::string_view ec_private_key2 {
  "MHQCAQEEIMhAttMFB2H70eWRmUrRqxzmr7Q0s6Oi5EzxlBKR/dCfoAcGBSuBBAAKoUQDQgAEzZAc8y92btejhFwuZfUvYNUjWIQUtPyEnHeeLjdtNCZXkN5d/7W2MHVsNZN5fW8CIQdrSWjPJGe//RXvFLakUg" };

std::string_view ec_public_key2 {
  "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEzZAc8y92btejhFwuZfUvYNUjWIQUtPyEnHeeLjdtNCZXkN5d/7W2MHVsNZN5fW8CIQdrSWjPJGe//RXvFLakUg" };

TEST_CASE("keypair_test", "[crypto]")
{
  SECTION("sign and verify")
  {
    ECSecp256k1PrivateKey private_key1(ec_private_key1);
    ECSecp256k1PublicKey public_key1(ec_public_key1);

    ECSecp256k1PrivateKey private_key2(ec_private_key2);
    ECSecp256k1PublicKey public_key2(ec_public_key2);

    auto hash { ECSecp256k1PrivateKey::hash_digest::from_string(
      "562d6ddfb3ceb5abb12d97bc35c4963d249f55b7c75eda618d365492ee98d469") };

    auto signature1 { private_key1.sign(hash) };
    CHECK(public_key1.verify(hash, signature1));
    CHECK(!public_key2.verify(hash, signature1));

    auto signature2 { private_key2.sign(hash) };
    CHECK(!public_key1.verify(hash, signature2));
    CHECK(public_key2.verify(hash, signature2));
  }
}
