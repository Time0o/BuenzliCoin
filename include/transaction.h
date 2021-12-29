#pragma once

#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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

    json to_json() const
    {
      json j;
      j["transaction_index"] = transaction_index;
      j["transaction_hash"] = transaction_hash.to_string();
      j["output_index"] = output_index;
      j["signature"] = signature.to_string();

      return j;
    }

    static TxI from_json(json const &j)
    {
      return {
        json_get(j, "transaction_index").get<std::size_t>(),
        hasher_digest::from_string(json_get(j, "transaction_hash")),
        json_get(j, "output_index").get<std::size_t>(),
        key_pair_digest::from_string(j["signature"])
      };
    }
  };

  struct TxO // Transaction output.
  {
    std::size_t amount; // Number of coins sent.
    std::string address; // Receiving wallet address.

    json to_json() const
    {
      json j;
      j["amount"] = amount;
      j["address"] = address;

      return j;
    }

    static TxO from_json(json const &j)
    {
      return {
        json_get(j, "amount").get<std::size_t>(),
        json_get(j, "address").get<std::string>()
      };
    }
  };

  struct UTxO // Unspent transaction output.
  {
    std::size_t transaction_index; // Index of transaction containing TxO.
    hasher_digest transaction_hash; // Hash of transaction containing TxO.
    std::size_t output_index; // Index of TxO in transaction.
    TxO output;

    json to_json() const
    {
      json j;
      j["transaction_index"] = transaction_index;
      j["transaction_hash"] = transaction_hash.to_string();
      j["output_index"] = output_index;
      j["output"] = output.to_json();

      return j;
    }
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

  bool valid() const
  {
    // XXX
    return true;
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

  json to_json() const
  {
    json j;

    switch (m_type) {
    case Type::STANDARD:
        j["type"] = "standard";
        break;
    case Type::REWARD:
        j["type"] = "reward";
        break;
    }

    j["index"] = m_index;

    j["hash"] = m_hash.to_string();

    for (auto const &txi : m_inputs)
      j["inputs"].push_back(txi.to_json());

    for (auto const &txo : m_outputs)
      j["outputs"].push_back(txo.to_json());

    return j;
  }

  static Transaction from_json(json const &j)
  {
    Type type;
    if (json_get(j, "type") == "standard")
      type = Type::STANDARD;
    else if (json_get(j, "type") == "reward")
      type = Type::REWARD;
    else
      throw std::logic_error("invalid transaction type");

    auto index { json_get(j, "index").get<std::size_t>() };

    auto hash { hasher_digest::from_string(json_get(j, "hash")) };

    std::vector<TxI> inputs;
    for (auto const &j_txi : json_get(j, "inputs"))
      inputs.emplace_back(TxI::from_json(j_txi));

    std::vector<TxO> outputs;
    for (auto const &j_txo : json_get(j, "outputs"))
      outputs.emplace_back(TxO::from_json(j_txo));

    return Transaction { type, index, hash, inputs, outputs };
  }

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

  void update_unspent_outputs() const
  {
    for (std::size_t i { 0 }; i < m_outputs.size(); ++i)
      (*m_unspent_outputs)[m_hash].emplace_back(m_index, m_hash, i, m_outputs[i]);

    for (auto const &txi : m_inputs) {
      auto it { m_unspent_outputs->find(txi.transaction_hash) };
      if (it != m_unspent_outputs->end()) {
        auto &l { it->second };

        auto it_remove {
          std::find_if(l.begin(),
                       l.end(),
                       [&txi](auto const &utxo)
                       { return utxo.output_index == txi.output_index; }) };

        assert(it_remove != l.end());

        l.erase(it_remove);
      }
    }
  }

  Type m_type;
  std::size_t m_index;
  hasher_digest m_hash;

  std::vector<TxI> m_inputs;
  std::vector<TxO> m_outputs;
  std::unique_ptr<UTxOMap> m_unspent_outputs;
};

} // end namespace bc
