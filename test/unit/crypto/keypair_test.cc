#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <string>

#include "crypto/keypair.h"

using namespace bc;

std::string ec_private_key1 {
R"(-----BEGIN EC PRIVATE KEY-----
MHQCAQEEILYZYhW4AeutWpQ9y5+jEY3YWR1Fohg0fdeEOow4CVVVoAcGBSuBBAAK
oUQDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0
/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w==
-----END EC PRIVATE KEY-----)"
};

std::string ec_public_key1 {
R"(-----BEGIN PUBLIC KEY-----
MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWf
n+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w==
-----END PUBLIC KEY-----)"
};

std::string ec_private_key2 {
R"(-----BEGIN EC PRIVATE KEY-----
MHQCAQEEIMhAttMFB2H70eWRmUrRqxzmr7Q0s6Oi5EzxlBKR/dCfoAcGBSuBBAAK
oUQDQgAEzZAc8y92btejhFwuZfUvYNUjWIQUtPyEnHeeLjdtNCZXkN5d/7W2MHVs
NZN5fW8CIQdrSWjPJGe//RXvFLakUg==
-----END EC PRIVATE KEY-----)"
};

std::string ec_public_key2 {
R"(-----BEGIN PUBLIC KEY-----
MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEzZAc8y92btejhFwuZfUvYNUjWIQUtPyE
nHeeLjdtNCZXkN5d/7W2MHVsNZN5fW8CIQdrSWjPJGe//RXvFLakUg==
-----END PUBLIC KEY-----)"
};

TEST_CASE("keypair_test", "[crypto]")
{
  SECTION("sign and verify")
  {
    ECPrivateKey private_key1 { ec_private_key1 };
    ECPublicKey public_key1 { ec_public_key1 };

    ECPrivateKey private_key2 { ec_private_key2 };
    ECPublicKey public_key2 { ec_public_key2 };

    auto message { "some message" };

    auto signature1 { private_key1.sign(message) };
    CHECK(public_key1.verify(message, signature1));
    CHECK(!public_key2.verify(message, signature1));

    auto signature2 { private_key2.sign(message) };
    CHECK(!public_key1.verify(message, signature2));
    CHECK(public_key2.verify(message, signature2));
  }
}
