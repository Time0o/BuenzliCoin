#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <openssl/evp.h>
#include <openssl/err.h>

#include "crypto/digest.h"

namespace bc
{

template<typename IMPL>
class Hasher
{
protected:
  Hasher() = default;

public:
  Digest hash(std::string_view msg) const
  {
    EVP_MD_CTX *mdctx { nullptr };

    uint8_t d_data[100];
    unsigned d_length;

    mdctx = EVP_MD_CTX_new();
    if (!(mdctx))
      throw std::bad_alloc {};

    if (EVP_DigestInit_ex(mdctx, IMPL::hasher(), nullptr) != 1)
      goto error;

    if (EVP_DigestUpdate(mdctx, msg.data(), msg.size()) != 1)
      goto error;

    if (EVP_DigestFinal_ex(mdctx, d_data, &d_length) != 1)
      goto error;

    EVP_MD_CTX_free(mdctx);

    return Digest { std::vector<uint8_t> { d_data, d_data + d_length } };

  error:
    if (mdctx)
      EVP_MD_CTX_free(mdctx);

    throw std::runtime_error(EVP_error());
  }

private:
  static char const *EVP_error()
  {
    char EVP_error_buf[120];

    return ERR_error_string(ERR_get_error(), EVP_error_buf);
  }
};

class SHA256Hasher : public Hasher<SHA256Hasher>
{
  friend class Hasher<SHA256Hasher>;

public:
  SHA256Hasher()
  : Hasher<SHA256Hasher>()
  {}

  static SHA256Hasher const &instance()
  {
    static SHA256Hasher hasher;
    return hasher;
  }

private:
  static EVP_MD const *hasher()
  { return EVP_sha256(); }
};

} // end namespace bc
