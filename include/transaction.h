#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "crypto/digest.h"
#include "crypto/hash.h"
#include "crypto/keypair.h"
#include "json.h"

namespace bc
{

// XXX Allow multiple transactions per block (i.e. transactions other than reward ones).
template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class Transaction
{
  using hasher_digest = typename HASHER::digest;
  using key_pair_digest = typename KEY_PAIR::digest;

public:
  enum class Type
  {
    STANDARD,
    REWARD
  };

  struct TxI // Transaction input.
  {
    std::size_t transaction_index; // Index of transaction containing TxO.
    hasher_digest transaction_hash; // Hash of transaction containing TxO.
    std::size_t output_index; // Index of TxO in transaction.
    KEY_PAIR::digest signature;

    json to_json() const;
    static TxI from_json(json const &j);
  };

  struct TxO // Transaction output.
  {
    std::size_t amount; // Number of coins sent.
    std::string address; // Receiving wallet address.

    json to_json() const;
    static TxO from_json(json const &j);
  };

  struct UTxO // Unspent transaction output.
  {
    std::size_t transaction_index; // Index of transaction containing TxO.
    hasher_digest transaction_hash; // Hash of transaction containing TxO.
    std::size_t output_index; // Index of TxO in transaction.
    TxO output;

    json to_json() const;
  };

  using UTxOList = std::list<UTxO>;
  using UTxOMap = std::unordered_map<hasher_digest, UTxOList>;

  UTxOList unspent_outputs() const
  {
    UTxOList list;

    for (auto const &[_, utxos] : *m_unspent_outputs)
      list.insert(list.end(), utxos.begin(), utxos.end());

    return list;
  }

  bool valid(std::size_t index) const
  {
    switch (m_type) {
    case Type::REWARD:
      return valid_reward(index);
    default:
      // XXX
      return true;
    }
  }

  void make_genesis()
  {
    m_unspent_outputs = std::make_unique<UTxOMap>();
    update_unspent_outputs();
  }

  void make_successor_of(Transaction &last)
  {
    m_unspent_outputs = std::move(last.m_unspent_outputs);
    update_unspent_outputs();
  }

  json to_json() const;
  static Transaction from_json(json const &j);

private:
  Transaction(Type type,
              std::size_t index,
              hasher_digest hash,
              std::vector<TxI> inputs,
              std::vector<TxO> outputs)
  : m_type { type }
  , m_index { index }
  , m_hash { std::move(hash) }
  , m_inputs { std::move(inputs) }
  , m_outputs { std::move(outputs) }
  {}

  bool valid_reward(std::size_t index) const;

  hasher_digest determine_hash() const;

  void update_unspent_outputs() const;

  Type m_type;
  std::size_t m_index;
  hasher_digest m_hash;

  std::vector<TxI> m_inputs;
  std::vector<TxO> m_outputs;
  std::unique_ptr<UTxOMap> m_unspent_outputs;
};

} // end namespace bc
