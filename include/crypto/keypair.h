#pragma once

#include <new>
#include <stdexcept>
#include <string>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "crypto/digest.h"

namespace bc
{

template<typename IMPL, typename TYPE, std::size_t DIGEST_LEN>
class PrivateKey
{
public:
  using digest = Digest<DIGEST_LEN>;

protected:
  PrivateKey(std::string const &key)
  : m_key(key)
  {}

public:
  digest sign(std::string const &msg) const
  {
    auto key { IMPL::read_key(m_key) };
    if (!key)
      throw std::runtime_error("failed to parse private key");

    EVP_MD_CTX *mdctx { nullptr };
    EVP_PKEY *pkey { nullptr };

    typename digest::array d;
    auto d_size { d.size() };

    mdctx = EVP_MD_CTX_new();
    if (!mdctx)
      throw std::bad_alloc {};

    pkey = EVP_PKEY_new();
    if (!pkey)
      throw std::bad_alloc {};

    if (IMPL::assign_key(pkey, key) != 1)
      goto error;

    if (EVP_DigestSignInit(mdctx, nullptr, IMPL::hasher(), nullptr, pkey) != 1)
      goto error;

    if (EVP_DigestSignUpdate(mdctx, msg.c_str(), msg.size()) != 1)
      goto error;

    if (EVP_DigestSignFinal(mdctx, d.data(), &d_size) != 1)
      goto error;

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    return digest { d };

  error:
    if (mdctx)
      EVP_MD_CTX_free(mdctx);

    if (pkey)
      EVP_PKEY_free(pkey);

    throw std::runtime_error { EVP_error() };
  }

private:
  static char const *EVP_error()
  {
    char EVP_error_buf[120];

    return ERR_error_string(ERR_get_error(), EVP_error_buf);
  }

  std::string m_key;
};

template<typename IMPL, typename TYPE, std::size_t DIGEST_LEN>
class PublicKey
{
public:
  using digest = Digest<DIGEST_LEN>;

protected:
  PublicKey(std::string const &key)
  : m_key(key)
  {}

public:
  bool verify(std::string const &msg, digest const &signature)
  {
    auto key { IMPL::read_key(m_key) };
    if (!key)
      throw std::runtime_error("failed to parse public key");

    EVP_MD_CTX *mdctx { nullptr };
    EVP_PKEY *pkey { nullptr };

    int status;
    bool verified;

    mdctx = EVP_MD_CTX_new();
    if (!mdctx)
      throw std::bad_alloc {};

    pkey = EVP_PKEY_new();
    if (!pkey)
      throw std::bad_alloc {};

    if (IMPL::assign_key(pkey, key) != 1)
      goto error;

    if (EVP_DigestVerifyInit(mdctx, nullptr, IMPL::hasher(), nullptr, pkey) != 1)
      goto error;

    if (EVP_DigestVerifyUpdate(mdctx, msg.c_str(), msg.size()) != 1)
      goto error;

    status = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.length());

    switch (status) {
      case 0:
        verified = false;
        break;
      case 1:
        verified = true;
        break;
        return true;
      default:
        goto error;
    }

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    return verified;

  error:
    if (mdctx)
      EVP_MD_CTX_free(mdctx);

    if (pkey)
      EVP_PKEY_free(pkey);

    throw std::runtime_error { EVP_error() };
  }

private:
  static char const *EVP_error()
  {
    char EVP_error_buf[120];

    return ERR_error_string(ERR_get_error(), EVP_error_buf);
  }

  std::string m_key;
};

class RSAPrivateKey : public PrivateKey<RSAPrivateKey, RSA, 128>
{
  friend class PrivateKey<RSAPrivateKey, RSA, 128>;

public:
  RSAPrivateKey(std::string const &key)
  : PrivateKey<RSAPrivateKey, RSA, 128>(key)
  {}

private:
  static RSA *read_key(std::string const &key)
  {
    auto bio { BIO_new_mem_buf(key.c_str(), -1) };
    if (!bio)
      throw std::bad_alloc {};

    return PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
  }

  static int assign_key(EVP_PKEY *parent_key, RSA *key)
  { return EVP_PKEY_assign_RSA(parent_key, key); }

  static EVP_MD const *hasher()
  { return EVP_sha256(); }
};

class RSAPublicKey : public PublicKey<RSAPublicKey, RSA, 128>
{
  friend class PublicKey<RSAPublicKey, RSA, 128>;

public:
  RSAPublicKey(std::string const &key)
  : PublicKey<RSAPublicKey, RSA, 128>(key)
  {}

private:
  static RSA *read_key(std::string const &key)
  {
    for (auto read : {PEM_read_bio_RSA_PUBKEY, PEM_read_bio_RSAPublicKey}) {
      auto bio { BIO_new_mem_buf(key.c_str(), -1) };
      if (!bio)
        throw std::bad_alloc {};

      RSA *key = read(bio, nullptr, nullptr, nullptr);
      if (key)
        return key;
    }

    return nullptr;
  }

  static int assign_key(EVP_PKEY *parent_key, RSA *key)
  { return EVP_PKEY_assign_RSA(parent_key, key); }

  static EVP_MD const *hasher()
  { return EVP_sha256(); }
};

} // end namespace bc
