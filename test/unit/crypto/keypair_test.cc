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

std::string rsa_private_key2 {
R"(-----BEGIN RSA PRIVATE KEY-----
MIICXAIBAAKBgQCGTjQncZi/poz3f7J/78tErbX3GX/tSko9BIVnnbrPWi5fp0Uf
GgM+qNTO8EC5MmVAm41zw4flN4ICT9wk8Xet5fWiD3fhRFpQ95gd7IEp5kYcLgtU
w3yegc+9H/ryyskjZaQ5S+RpzDt2HNKrXFo3rFiz1jqrCr8ahxsifRSsFQIDAQAB
AoGAEUBGTrK2rhdUkA3k/a3tbBrr/ptV5ULfmkrUX+TQtAWfY0X3CPGsbQX+n74Y
cjFY9B2G49G/yN+CYY2kd1JUzt6/OuR2KvY6KB0mEFulGR6pZcOFYmTVLH3+g5Nn
ldk0yJTE+X7M/cU3czsaI2LSuzmrmY+XDYp/Pdnd+5sJhkECQQDlNkYlmusSCap/
HBU1QSIuBwS5X45i0iqkILoG/hRoiM1rc3LXB8tblUEK0eia7IyX0uoFimnzZixV
49a2mSO9AkEAlgBzGJphDIKySo729vKFBTs/NJcoqNBdgjzNJqMtyxbclxBo60R8
UALyg6NDLrjXxawRnB6cpF6yrLCXWJqDOQJAMY4nYk/5DKBMXUjcCPR5CEx6J/3R
0emwUGXG8mYSUXtqNhXyuSy1OaquMkGpsXz89IIkGGiReY8YfMVSkY3QXQJAfLg0
+qDpzngg/CMwrpVpCrd/Tx/b27kb9rzNKrIE0lbY5PXs9qkD35cDw3YM1x0zsxTl
s3Q0c+qVD0bpH++g6QJBAKWJZVMfhHDtgA2oZZyi7KE7/JU7bYUOAXX8khPVh9zf
YbMOaFBe7Berzc+W4lz5b0CNNqswvY/xfB07MTnibtM=
-----END RSA PRIVATE KEY-----)"
};

std::string rsa_public_key2 {
R"(-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCGTjQncZi/poz3f7J/78tErbX3
GX/tSko9BIVnnbrPWi5fp0UfGgM+qNTO8EC5MmVAm41zw4flN4ICT9wk8Xet5fWi
D3fhRFpQ95gd7IEp5kYcLgtUw3yegc+9H/ryyskjZaQ5S+RpzDt2HNKrXFo3rFiz
1jqrCr8ahxsifRSsFQIDAQAB
-----END PUBLIC KEY-----)"
};

TEST_CASE("keypair_test", "[crypto]")
{
  SECTION("sign and verify")
  {
    RSAPrivateKey private_key1 { rsa_private_key1 };
    RSAPublicKey public_key1 { rsa_public_key1 };

    RSAPrivateKey private_key2 { rsa_private_key2 };
    RSAPublicKey public_key2 { rsa_public_key2 };

    auto message { "some message" };

    auto signature1 { private_key1.sign(message) };
    CHECK(public_key1.verify(message, signature1));
    CHECK(!public_key2.verify(message, signature1));

    auto signature2 { private_key2.sign(message) };
    CHECK(!public_key1.verify(message, signature2));
    CHECK(public_key2.verify(message, signature2));
  }
}
