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

template<typename IMPL, std::size_t HASH_DIGEST_LEN, std::size_t SIG_DIGEST_LEN>
class PrivateKey
{
public:
  using hash_digest = Digest<HASH_DIGEST_LEN>;
  using sig_digest = Digest<SIG_DIGEST_LEN>;

protected:
  PrivateKey(std::string_view key)
  : m_key { detail::build_key(key, IMPL::header(), IMPL::footer()) }
  {}

public:
  sig_digest sign(hash_digest const &hash) const
  {
    auto key { IMPL::read_key(m_key) };
    if (!key)
      throw std::runtime_error("failed to parse private key");

    EVP_PKEY_CTX *pkey_ctx { nullptr };
    EVP_PKEY *pkey { nullptr };

    sig_digest sig;
    auto sig_length { sig.length() };

    pkey = EVP_PKEY_new();
    if (!pkey)
      throw std::bad_alloc {};

    if (IMPL::assign_key(pkey, key) != 1)
      goto error;

    pkey_ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!pkey_ctx)
      throw std::bad_alloc {};

    if (EVP_PKEY_sign_init(pkey_ctx) != 1)
      goto error;

    if (EVP_PKEY_sign(pkey_ctx, sig.data(), &sig_length, hash.data(), hash.length()) != 1)
      goto error;

    sig.adjust_length(sig_length);

    EVP_PKEY_CTX_free(pkey_ctx);
    EVP_PKEY_free(pkey);

    return sig;

  error:
    std::string error { ERR_error_string(ERR_get_error(), nullptr) };

    if (pkey_ctx)
      EVP_PKEY_CTX_free(pkey_ctx);

    if (pkey)
      EVP_PKEY_free(pkey);

    throw std::runtime_error { error };
  }

private:
  std::string m_key;
};

template<typename IMPL, std::size_t HASH_DIGEST_LEN, std::size_t SIG_DIGEST_LEN>
class PublicKey
{
public:
  using hash_digest = Digest<HASH_DIGEST_LEN>;
  using sig_digest = Digest<SIG_DIGEST_LEN>;

protected:
  PublicKey(std::string_view key)
  : m_key { detail::build_key(key, IMPL::header(), IMPL::footer()) }
  {}

public:
  bool verify(hash_digest const &hash, sig_digest const &sig) const
  {
    auto key { IMPL::read_key(m_key) };
    if (!key)
      throw std::runtime_error("failed to parse public key");

    EVP_PKEY_CTX *pkey_ctx { nullptr };
    EVP_PKEY *pkey { nullptr };

    int status;
    bool verified;

    pkey = EVP_PKEY_new();
    if (!pkey)
      throw std::bad_alloc {};

    if (IMPL::assign_key(pkey, key) != 1)
      goto error;

    pkey_ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!pkey_ctx)
      throw std::bad_alloc {};

    if (EVP_PKEY_verify_init(pkey_ctx) != 1)
      goto error;

    status = EVP_PKEY_verify(pkey_ctx, sig.data(), sig.length(), hash.data(), hash.length());

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

    EVP_PKEY_CTX_free(pkey_ctx);
    EVP_PKEY_free(pkey);

    return verified;

  error:
    std::string error { ERR_error_string(ERR_get_error(), nullptr) };

    if (pkey_ctx)
      EVP_PKEY_CTX_free(pkey_ctx);

    if (pkey)
      EVP_PKEY_free(pkey);

    throw std::runtime_error { error };
  }

private:
  std::string m_key;
};

template<typename PRIVATE_KEY, typename PUBLIC_KEY>
struct KeyPair
{
  using private_key = PRIVATE_KEY;
  using public_key = PUBLIC_KEY;

  using hash_digest = PRIVATE_KEY::hash_digest;
  using sig_digest = PRIVATE_KEY::sig_digest;
};

class ECSecp256k1PrivateKey : public PrivateKey<ECSecp256k1PrivateKey, 32, 72>
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
};

class ECSecp256k1PublicKey : public PublicKey<ECSecp256k1PublicKey, 32, 72>
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
};

using ECSecp256k1KeyPair = KeyPair<ECSecp256k1PrivateKey, ECSecp256k1PublicKey>;

} // end namespace bc
