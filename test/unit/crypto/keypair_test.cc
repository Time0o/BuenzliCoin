#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <string>

#include "crypto/keypair.h"

using namespace bc;

std::string rsa_private_key1 {
R"(-----BEGIN RSA PRIVATE KEY-----
MIICWgIBAAKBgH9ZR6x565t6DFbPYt17hq4DOWiC1dbJmatLZbPTgDVaMl/KZuFM
kKT/bxlIfuCOvFL1b1O/q60cEBiharetA4Xiu+VH7B8Nujr+1jl5RVEKzYq/5uhG
u7EmXDEhy2dz95oy6tIM7ezg/V06T9fuiweu+IHz4AicMao8Eaf6eGgzAgMBAAEC
gYAtJDmq4uDokP/UudjCx+m6vzvXVyFz7KyDMsvPTbaRsvJOUFHdpSVUx5LbNH59
HTuWcJfQ6Q1y5JK0GGvaxgKML5yMzqBfJ8019lSV+fX3cImwWZrdHtK73fCqDmbN
gs8eFUReOl/mVdStKC/Qu5pJeZEsIUmf59p179OVma1EAQJBAMQV3PCbsmP+XvWp
K/vZDQTrVsMmtVDK/RP06svYNmEjzSRa0RpeJTg6uEbJ+w7GwV6HmHZNQjhVjfjq
izdZnjMCQQCmQrfI4b6SK2dzrQhkjUxs7Y3+UwHfAn4iPnUlnm292Nln4jpJQK7s
PsViSGU9LlP1UpP43Xb594eljbDJqA4BAkBrL0hGfdVVs4ZU4tSYJl1ngv06T75t
G4ibkBWIt/eBwgAxDzOeJjhSNEbm6yHBQgQRmC1O/YxlHt8sYYkYCHThAkB7CXae
Z0izPQGi3hMO7m33Ulk5M054LY0QZG4m97Y4vygsM4N0wDRyygUiNXcOLqGdM44j
piis1VyBzHhe00oBAkACCtQFGA+Fu00B5XSAlMyVxDoO+v9qggh3pYfKKS8O7f7j
SS7cZydY/lP4j+2HKkNinpc17b+9SinULVCKwgmA
-----END RSA PRIVATE KEY-----)"
};

std::string rsa_public_key1 {
R"(-----BEGIN PUBLIC KEY-----
MIGeMA0GCSqGSIb3DQEBAQUAA4GMADCBiAKBgH9ZR6x565t6DFbPYt17hq4DOWiC
1dbJmatLZbPTgDVaMl/KZuFMkKT/bxlIfuCOvFL1b1O/q60cEBiharetA4Xiu+VH
7B8Nujr+1jl5RVEKzYq/5uhGu7EmXDEhy2dz95oy6tIM7ezg/V06T9fuiweu+IHz
4AicMao8Eaf6eGgzAgMBAAE=
-----END PUBLIC KEY-----)"
};

TEST_CASE("keypair_test", "[crypto]")
{
  SECTION("sign and verify")
  {
    RSAPrivateKey private_key { rsa_private_key1 };

    auto digest { private_key.sign("some message") };

    // XXX Verify
  }
}
