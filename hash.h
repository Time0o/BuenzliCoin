#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>

#include <openssl/evp.h>
#include <openssl/err.h>

namespace bm
{

template<unsigned DIGEST_LEN>
class Hasher
{
public:
  using digest = std::array<unsigned char, DIGEST_LEN>;

protected:
  Hasher(EVP_MD const *(*md)())
  : m_mdctx(EVP_MD_CTX_new()),
    m_md(md)
  {
    if (!(m_mdctx))
      throw std::bad_alloc {};

    if (DIGEST_LEN != EVP_MD_size(m_md()))
      throw std::logic_error("mismatched digest lengths");
  }

public:
  ~Hasher()
  {
    EVP_MD_CTX_free(m_mdctx);
  }

  digest hash(std::string const &msg) const
  {
    unsigned char d_c[DIGEST_LEN];
    unsigned d_c_len = 0;

    if (EVP_DigestInit_ex(m_mdctx, m_md(), nullptr) != 1)
    	goto error;

    if (EVP_DigestUpdate(m_mdctx, msg.data(), msg.size()) != 1)
    	goto error;

    if(EVP_DigestFinal_ex(m_mdctx, d_c, &d_c_len) != 1)
    	goto error;

    EVP_MD_CTX_reset(m_mdctx);

    digest d;
    std::copy(std::begin(d_c), std::end(d_c), d.begin());

    return d;

  error:
    EVP_MD_CTX_reset(m_mdctx);

    throw std::runtime_error(EVP_error());
  }

private:
  static char const *EVP_error()
  {
    char EVP_error_buf[120];

    return ERR_error_string(ERR_get_error(), EVP_error_buf);
  }

  EVP_MD const *(*m_md)() {nullptr};
  EVP_MD_CTX *m_mdctx {nullptr};
};

struct SHA256Hasher : Hasher<32u>
{
  SHA256Hasher()
  : Hasher<32u>(EVP_sha256)
  {}

  static SHA256Hasher const &instance()
  {
    static SHA256Hasher hasher;
    return hasher;
  }
};

} // end namespace bm
