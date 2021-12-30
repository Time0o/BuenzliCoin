#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.h"
#include "transaction.h"

namespace bc
{

template<typename KEY_PAIR, typename HASHER>
json
Transaction<KEY_PAIR, HASHER>::TxI::to_json() const
{
  json j;
  j["transaction_index"] = transaction_index;
  j["transaction_hash"] = transaction_hash.to_string();
  j["output_index"] = output_index;
  j["signature"] = signature.to_string();

  return j;
}

template json Transaction<>::TxI::to_json() const;

template<typename KEY_PAIR, typename HASHER>
Transaction<KEY_PAIR, HASHER>::TxI
Transaction<KEY_PAIR, HASHER>::TxI::from_json(json const &j)
{
  return {
    json_get(j, "transaction_index").get<std::size_t>(),
    hasher_digest::from_string(json_get(j, "transaction_hash")),
    json_get(j, "output_index").get<std::size_t>(),
    key_pair_digest::from_string(json_get(j, "signature"))
  };
}

template Transaction<>::TxI Transaction<>::TxI::from_json(json const &data);

template<typename KEY_PAIR, typename HASHER>
json
Transaction<KEY_PAIR, HASHER>::TxO::to_json() const
{
  json j;
  j["amount"] = amount;
  j["address"] = address;

  return j;
}

template json Transaction<>::TxO::to_json() const;

template<typename KEY_PAIR, typename HASHER>
Transaction<KEY_PAIR, HASHER>::TxO
Transaction<KEY_PAIR, HASHER>::TxO::from_json(json const &j)
{
  return {
    json_get(j, "amount").get<std::size_t>(),
    json_get(j, "address").get<std::string>()
  };
}

template Transaction<>::TxO Transaction<>::TxO::from_json(json const &data);

template<typename KEY_PAIR, typename HASHER>
json
Transaction<KEY_PAIR, HASHER>::UTxO::to_json() const
{
  json j;
  j["transaction_index"] = transaction_index;
  j["transaction_hash"] = transaction_hash.to_string();
  j["output_index"] = output_index;
  j["output"] = output.to_json();

  return j;
}

template json Transaction<>::UTxO::to_json() const;

template<typename KEY_PAIR, typename HASHER>
json
Transaction<KEY_PAIR, HASHER>::to_json() const
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

template json Transaction<>::to_json() const;

template<typename KEY_PAIR, typename HASHER>
Transaction<KEY_PAIR, HASHER>
Transaction<KEY_PAIR, HASHER>::from_json(json const &j)
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

template Transaction<> Transaction<>::from_json(json const &data);

template<typename KEY_PAIR, typename HASHER>
bool
Transaction<KEY_PAIR, HASHER>::valid_reward(std::size_t index) const
{
  if (m_index != index)
    return false;

  if (m_hash != determine_hash())
    return false;

  if (!m_inputs.empty())
    return false;

  if (m_outputs.size() != 1)
    return false;

  if (m_outputs[0].amount != config().transaction_reward_amount)
    return false;

  return true;
}

template bool Transaction<>::valid_reward(std::size_t index) const;

template<typename KEY_PAIR, typename HASHER>
Transaction<KEY_PAIR, HASHER>::hasher_digest
Transaction<KEY_PAIR, HASHER>::determine_hash() const
{
  std::stringstream ss;

  ss << m_index;

  for (auto const &txi : m_inputs)
    ss << txi.transaction_index
       << txi.transaction_hash.to_string()
       << txi.output_index;

  for (auto const &txo : m_outputs)
    ss << txo.amount
       << txo.address;

  return HASHER::instance().hash(ss.str());
}

template Transaction<>::hasher_digest Transaction<>::determine_hash() const;

template<typename KEY_PAIR, typename HASHER>
void
Transaction<KEY_PAIR, HASHER>::update_unspent_outputs() const
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

template void Transaction<>::update_unspent_outputs() const;

} // end namespace bc
