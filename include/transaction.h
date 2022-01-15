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
    HASHER::digest output_hash; // Hash of transaction containing TxO.
    std::size_t output_index; // Index of TxO in transaction.
    KEY_PAIR::digest signature;

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
    HASHER::digest output_hash; // Hash of transaction containing TxO.
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

  static Transaction reward(std::string const &reward_address, std::size_t index)
  {
    Transaction t {
      Type::REWARD,
      index,
      {},
      {},
      { TxO { config().transaction_reward_amount, reward_address } }
    };

    t.m_hash = t.determine_hash();

    return t;
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

  std::pair<bool, std::string> valid_standard() const;
  std::pair<bool, std::string> valid_reward() const;

  digest determine_hash() const;

  Type m_type;
  std::size_t m_index;
  digest m_hash;

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

  // XXX Source file.
  void update(transaction const &t)
  {
    auto const &hash { t.hash() };
    auto const &inputs { t.inputs() };
    auto const &outputs { t.outputs() };

    for (std::size_t i { 0 }; i < outputs.size(); ++i)
      m_unspent_outputs.emplace_back(hash, i, outputs[i]);

    for (auto const &txi : inputs) {
      for (auto it { m_unspent_outputs.begin() }; it != m_unspent_outputs.end(); ++it) {
        if (it->output_hash == txi.output_hash &&
            it->output_index == txi.output_index) {

          it = m_unspent_outputs.erase(it);
        }
      }
    }
  }

  // XXX Source file
  json to_json() const
  {
    json j = json::array();
    for (auto const &utxo : m_unspent_outputs)
      j.push_back(utxo.to_json());

    return j;
  }

private:
  std::list<typename transaction::unspent_output> m_unspent_outputs;
};

// TODO 1: Add POST endpoint that just adds to pool [x]
// TODO 2: Return pool via GET endpoint [x]
// TODO 3: Construct new blocks from pool [x]
// TODO 4: Propagate pool between nodes
// TODO 5: Update pool on every new block [x]
template<typename KEY_PAIR = ECSecp256k1KeyPair, typename HASHER = SHA256Hasher>
class TransactionUnconfirmedPool
{
  using transaction = Transaction<KEY_PAIR, HASHER>;
  using transaction_list = TransactionList<KEY_PAIR, HASHER>;

public:
  std::list<transaction> const &get()
  { return m_transactions; }

  // XXX Source file.
  bool empty() const
  { return m_transactions.empty(); }

  transaction next()
  {
    auto t { m_transactions.front() };

    m_transactions.pop_front();

    return t;
  }

  void add(transaction const &t)
  {
    auto [valid, error] = t.valid();

    if (!valid)
      throw std::runtime_error(
        fmt::format("attempted to add invalid transaction to pool: {}", error));

    for (auto const &t_ : m_transactions) {
      for (auto const &txi : t.inputs()) {
        for (auto const &txi_ : t_.inputs()) {
          if (txi == txi_)
            throw std::runtime_error(
              "Attempted to add invalid transaction to pool: duplicate inputs");
        }
      }
    }

    m_transactions.push_back(t);
  }

  void remove(transaction const &t)
  {
    for (auto it { m_transactions.begin() }; it != m_transactions.end(); ++it) {
      if (it->hash() == t.hash())
        it = m_transactions.erase(it);
    }
  }

  void prune(std::list<typename transaction::unspent_output> const &unspent_outputs)
  {
    for (auto it { m_transactions.begin() }; it != m_transactions.end(); ++it) {
      for (auto const &txi : it->inputs()) {
        bool txi_valid { false };
        for (auto const &utxo : unspent_outputs) {
          if (utxo == txi) {
            txi_valid = true;
            break;
          }
        }

        if (!txi_valid)
          it = m_transactions.erase(it);
      }
    }
  }

  // XXX Source file
  json to_json() const
  {
    json j = json::array();
    for (auto const &t : m_transactions)
      j.push_back(t.to_json());

    return j;
  }

  std::list<transaction> m_transactions;
};

} // end namespace bc
