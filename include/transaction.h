#pragma once

#include <cstdint>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "crypto/digest.h"
#include "crypto/hash.h"
#include "crypto/keypair.h"
#include "format.h"
#include "json.h"

namespace bc
{

template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class Transaction
{
  struct TxI // Transaction input.
  {
    Digest output_hash; // Hash of transaction containing TxO.
    std::size_t output_index; // Index of TxO in transaction.
    Digest signature;

    bool operator==(TxI const &other) const
    {
      return output_hash == other.output_hash &&
             output_index == other.output_index;
    }

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
    Digest output_hash; // Hash of transaction containing TxO.
    std::size_t output_index; // Index of TxO in transaction.
    TxO output;

    bool operator==(UTxO const &other) const
    {
      return output_hash == other.output_hash &&
             output_index == other.output_index;
    }

    bool operator==(TxI const &other) const
    {
      return output_hash == other.output_hash &&
             output_index == other.output_index;
    }

    json to_json() const;
  };

public:
  enum class Type
  {
    STANDARD,
    REWARD
  };

  using input = TxI;
  using output = TxO;
  using unspent_output = UTxO;

  Type type() const
  { return m_type; }

  std::size_t index() const
  { return m_index; }

  Digest hash() const
  { return m_hash; }

  std::vector<input> const &inputs() const
  { return m_inputs; }

  std::vector<output> const &outputs() const
  { return m_outputs; }

  std::list<unspent_output> const &unspent_outputs() const
  { return m_unspent_outputs; }

  void update_unspent_outputs(std::list<unspent_output> unspent_outputs)
  { m_unspent_outputs = std::move(unspent_outputs); }

  std::pair<bool, std::string> valid() const
  {
    switch (m_type) {
    case Type::REWARD:
      return valid_reward();
    default:
      return valid_standard();
    }
  }

  static Transaction reward(std::string const &reward_address, std::size_t index);

  json to_json() const;
  static Transaction from_json(json const &j);

private:
  Transaction(Type type,
              std::size_t index,
              Digest hash,
              std::vector<TxI> inputs,
              std::vector<TxO> outputs)
  : m_type { type }
  , m_index { index }
  , m_hash { std::move(hash) }
  , m_inputs { std::move(inputs) }
  , m_outputs { std::move(outputs) }
  {}

  std::pair<bool, std::string> valid_standard() const;
  std::pair<bool, std::string> valid_reward() const;

  Digest determine_hash() const;

  Type m_type;
  std::size_t m_index;
  Digest m_hash;

  std::vector<input> m_inputs;
  std::vector<output> m_outputs;
  std::list<unspent_output> m_unspent_outputs;
};

template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class TransactionList
{
  using transaction = Transaction<KEY_PAIR, HASHER>;

public:
  template<typename IT>
  TransactionList(IT start, IT end)
  : m_transactions { start, end }
  {}

  std::vector<transaction> &get()
  { return m_transactions; }

  std::vector<transaction> const &get() const
  { return m_transactions; }

  std::pair<bool, std::string> valid(std::size_t index) const;

  json to_json() const;
  static TransactionList from_json(json const &j);

private:
  std::vector<transaction> m_transactions;
};

template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class TransactionUnspentOutputs
{
  using transaction = Transaction<KEY_PAIR, HASHER>;
  using transaction_list = TransactionList<KEY_PAIR, HASHER>;

public:
  std::list<typename transaction::unspent_output> const &get() const
  { return m_unspent_outputs; }

  void update(transaction const &t);

  void clear()
  { m_unspent_outputs.clear(); }

  json to_json() const;

private:
  std::list<typename transaction::unspent_output> m_unspent_outputs;
};

template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class TransactionUnconfirmedPool
{
  using transaction = Transaction<KEY_PAIR, HASHER>;
  using transaction_list = TransactionList<KEY_PAIR, HASHER>;

public:
  bool empty() const
  { return m_transactions.empty(); }

  std::list<transaction> const &get()
  { return m_transactions; }

  transaction next();

  void add(transaction const &t);

  void remove(transaction const &t);

  void prune(std::list<typename transaction::unspent_output> const &unspent_outputs);

  void clear()
  { m_transactions.clear(); }

  json to_json() const;

private:
  std::list<transaction> m_transactions;
};

} // end namespace bc
