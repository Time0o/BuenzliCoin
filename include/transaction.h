#pragma once

#include <cstdint>
#include <sstream>
#include <string>

#include "crypto/digest.h"
#include "crypto/hash.h"
#include "crypto/keypair.h"

namespace bc
{

template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class Transaction
{
public:
  struct In
  {
    HASHER::digest out_id;
    std::size_t out_index;

    KEY_PAIR::digest signature;
  };

  struct Out
  {
    std::string addr;
    std::size_t amount;
  };

private:
  HASHER::digest determine_hash() const
  {
    std::stringstream ss;

    for (auto const &in : m_ins)
      ss << in.out_id << in.out_index;

    for (auto const &out : m_outs)
      ss << out.addr << out.amount;

    return HASHER::instance().hash(ss.str());
  }

  std::vector<In> m_ins;
  std::vector<Out> m_outs;

  HASHER::digest m_hash;
};

} // end namespace bc
