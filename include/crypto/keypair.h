#pragma once

#include <new>
#include <stdexcept>
#include <string>
#include <string_view>

#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include "crypto/digest.h"

namespace bc
{

namespace detail
{

inline std::string build_key(std::string_view key_,
                             std::string_view header,
                             std::string_view footer)
{
  std::string key { key_ };

  while (key.length() % 4 != 0)
    key.push_back('=');

  std::stringstream ss;

  ss << header << '\n'
     << key << '\n'
     << footer;

  return ss.str();
}

} // end namespace detail

template<typename IMPL, typename TYPE, std::size_t DIGEST_LEN>
class PrivateKey
{
public:
  using digest = Digest<DIGEST_LEN>;

protected:
  PrivateKey(std::string_view key)
  : m_key { detail::build_key(key, IMPL::header(), IMPL::footer()) }
  {}

public:
  digest sign(std::string_view msg) const
  {
    auto key { IMPL::read_key(m_key) };
    if (!key)
      throw std::runtime_error("failed to parse private key");

    EVP_MD_CTX *mdctx { nullptr };
    EVP_PKEY *pkey { nullptr };

    digest signature;
    auto signature_length { signature.length() };

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

    if (EVP_DigestSignUpdate(mdctx, msg.data(), msg.length()) != 1)
      goto error;

    if (EVP_DigestSignFinal(mdctx, signature.data(), &signature_length) != 1)
      goto error;

    signature.adjust_length(signature_length);

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    return signature;

  error:
    std::string error { ERR_error_string(ERR_get_error(), nullptr) };

    if (mdctx)
      EVP_MD_CTX_free(mdctx);

    if (pkey)
      EVP_PKEY_free(pkey);

    throw std::runtime_error { error };
  }

private:
  std::string m_key;
};

template<typename IMPL, typename TYPE, std::size_t DIGEST_LEN>
class PublicKey
{
public:
  using digest = Digest<DIGEST_LEN>;

protected:
  PublicKey(std::string_view key)
  : m_key { detail::build_key(key, IMPL::header(), IMPL::footer()) }
  {}

public:
  bool verify(std::string_view msg, digest const &d) const
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

    if (EVP_DigestVerifyUpdate(mdctx, msg.data(), msg.length()) != 1)
      goto error;

    status = EVP_DigestVerifyFinal(mdctx, d.data(), d.length());

    switch (status) {
      case 0:
        verified = false;
        break;
      case 1:
        verified = true;
        break;
      default:
        goto error;
    }

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    return verified;

  error:
    std::string error { ERR_error_string(ERR_get_error(), nullptr) };

    if (mdctx)
      EVP_MD_CTX_free(mdctx);

    if (pkey)
      EVP_PKEY_free(pkey);

    throw std::runtime_error { error };
  }

private:
  std::string m_key;
};

template<typename PRIVATE_KEY, typename PUBLIC_KEY, std::size_t DIGEST_LEN>
struct KeyPair
{
  using private_key = PRIVATE_KEY;
  using public_key = PUBLIC_KEY;

  using digest = Digest<DIGEST_LEN>;
};

class ECSecp256k1PrivateKey : public PrivateKey<ECSecp256k1PrivateKey, EC_KEY, 72>
{
  friend class PrivateKey;

public:
  ECSecp256k1PrivateKey(std::string_view key)
  : PrivateKey(key)
  {}

private:
  static std::string_view header()
  { return "-----BEGIN EC PRIVATE KEY-----"; }

  static std::string_view footer()
  { return "-----END EC PRIVATE KEY-----"; }

  static EC_KEY *read_key(std::string_view key)
  {
    auto bio { BIO_new_mem_buf(key.data(), key.length()) };
    if (!bio)
      throw std::bad_alloc {};

    return PEM_read_bio_ECPrivateKey(bio, nullptr, nullptr, nullptr);
  }

  static int assign_key(EVP_PKEY *parent_key, EC_KEY *key)
  { return EVP_PKEY_assign_EC_KEY(parent_key, key); }

  static EVP_MD const *hasher()
  { return EVP_sha256(); }
};

class ECSecp256k1PublicKey : public PublicKey<ECSecp256k1PublicKey, EC_KEY, 72>
{
  friend class PublicKey;

public:
  ECSecp256k1PublicKey(std::string_view key)
  : PublicKey(key)
  {}

private:
  static std::string_view header()
  { return "-----BEGIN PUBLIC KEY-----"; }

  static std::string_view footer()
  { return "-----END PUBLIC KEY-----"; }

  static EC_KEY *read_key(std::string_view key)
  {
    auto bio { BIO_new_mem_buf(key.data(), key.length()) };
    if (!bio)
      throw std::bad_alloc {};

    return PEM_read_bio_EC_PUBKEY(bio, nullptr, nullptr, nullptr);
  }

  static int assign_key(EVP_PKEY *parent_key, EC_KEY *key)
  { return EVP_PKEY_assign_EC_KEY(parent_key, key); }

  static EVP_MD const *hasher()
  { return EVP_sha256(); }
};

using ECSecp256k1KeyPair = KeyPair<ECSecp256k1PrivateKey, ECSecp256k1PublicKey, 72>;

} // end namespace bc
