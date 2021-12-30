#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "crypto/digest.h"
#include "crypto/hash.h"
#include "crypto/keypair.h"
#include "json.h"

namespace bc
{

template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class Transaction
{
  struct TxI // Transaction input.
  {
    HASHER::digest output_hash; // Hash of transaction containing TxO.
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
    HASHER::digest output_hash; // Hash of transaction containing TxO.
    std::size_t output_index; // Index of TxO in transaction.
    TxO output;

    json to_json() const;
    static UTxO from_json(json const &j); // XXX
  };

public:
  enum class Type
  {
    STANDARD,
    REWARD
  };

  using digest = typename HASHER::digest;

  using input = TxI;
  using output = TxO;
  using unspent_output = UTxO;

  Type type() const
  { return m_type; }

  std::size_t index() const
  { return m_index; }

  digest hash() const
  { return m_hash; }

  std::vector<input> const &inputs() const
  { return m_inputs; }

  std::vector<output> const &outputs() const
  { return m_outputs; }

  bool valid() const
  {
    switch (m_type) {
    case Type::REWARD:
      return valid_reward();
    default:
      return true; // XXX
    }
  }

  json to_json() const;
  static Transaction from_json(json const &j);

private:
  Transaction(Type type,
              std::size_t index,
              digest hash,
              std::vector<TxI> inputs,
              std::vector<TxO> outputs)
  : m_type { type }
  , m_index { index }
  , m_hash { std::move(hash) }
  , m_inputs { std::move(inputs) }
  , m_outputs { std::move(outputs) }
  {}

  bool valid_reward() const;

  digest determine_hash() const;

  Type m_type;
  std::size_t m_index;
  digest m_hash;

  std::vector<input> m_inputs;
  std::vector<output> m_outputs;
};

template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class TransactionGroup
{
public:
  using transaction = Transaction<KEY_PAIR, HASHER>;

  using output = transaction::output;
  using unspent_output = transaction::unspent_output;

  output const &reward_output() const
  { return m_transactions[0].outputs()[0]; }

  std::list<unspent_output> unspent_outputs() const
  { return *m_unspent_outputs; }

  bool valid(std::size_t index) const;

  void make_genesis()
  {
    m_unspent_outputs = std::make_unique<std::list<unspent_output>>();
    update_unspent_outputs();
  }

  void make_successor_of(TransactionGroup &last)
  {
    m_unspent_outputs = std::move(last.m_unspent_outputs);
    update_unspent_outputs();
  }

  json to_json() const;
  static TransactionGroup from_json(json const &j);

private:
  TransactionGroup(std::vector<transaction> transactions)
  : m_transactions { std::move(transactions) }
  {}

  void update_unspent_outputs() const;

  std::vector<transaction> m_transactions;

  std::unique_ptr<std::list<unspent_output>> m_unspent_outputs;
};

} // end namespace bc
