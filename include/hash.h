#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <openssl/evp.h>
#include <openssl/err.h>

namespace bc
{

template<std::size_t DIGEST_LEN>
class Digest
{
public:
  using array = std::array<uint8_t, DIGEST_LEN>;

  Digest(array arr)
  : m_arr { std::move(arr) }
  {}

  bool operator==(Digest const &other) const
  { return m_arr == other.m_arr; }

  std::string to_string() const
  {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (auto const &byte : m_arr)
      ss << std::setw(2) << static_cast<int>(byte);

    return ss.str();
  }

  static Digest from_string(std::string const &str)
  {
    if (str.size() != std::tuple_size_v<array> * 2)
      throw std::invalid_argument("invalid digest string");

    auto char_to_nibble = [](char c){
      if (c >= '0' && c <= '9')
        return c - '0';

      if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

      if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

      throw std::invalid_argument("invalid digest string");
    };

    array d;
    for (std::size_t i = 0; i < d.size(); ++i)
      d[i] = (char_to_nibble(str[2 * i]) << 4) | char_to_nibble(str[2 * i + 1]);

    return Digest { d };
  }

private:
  array m_arr;
};

template<std::size_t DIGEST_LEN>
class Hasher
{
public:
  using digest = Digest<DIGEST_LEN>;

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
    uint8_t d_c[DIGEST_LEN];
    unsigned d_c_len = 0;

    if (EVP_DigestInit_ex(m_mdctx, m_md(), nullptr) != 1)
    	goto error;

    if (EVP_DigestUpdate(m_mdctx, msg.data(), msg.size()) != 1)
    	goto error;

    if(EVP_DigestFinal_ex(m_mdctx, d_c, &d_c_len) != 1)
    	goto error;

    EVP_MD_CTX_reset(m_mdctx);

    typename digest::array d;
    std::copy(std::begin(d_c), std::end(d_c), d.begin());

    return digest { d };

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

} // end namespace bc
