#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
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
  j["output_hash"] = output_hash.to_string();
  j["output_index"] = output_index;
  j["signature"] = signature.to_string();

  return j;
}

template json Transaction<>::TxI::to_json() const;

template<typename KEY_PAIR, typename HASHER>
Transaction<KEY_PAIR, HASHER>::TxI
Transaction<KEY_PAIR, HASHER>::TxI::from_json(json const &j)
{
  auto output_hash { HASHER::digest::from_string(json_get(j, "output_hash")) };
  auto output_index { json_get(j, "output_index").get<std::size_t>() };
  auto signature { KEY_PAIR::digest::from_string(json_get(j, "signature")) };

  return { output_hash, output_index, signature };
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
  auto amount { json_get(j, "amount").get<std::size_t>() };
  auto address { json_get(j, "address").get<std::string>() };

  return { amount, address };
}

template Transaction<>::TxO Transaction<>::TxO::from_json(json const &data);

template<typename KEY_PAIR, typename HASHER>
json
Transaction<KEY_PAIR, HASHER>::UTxO::to_json() const
{
  json j;
  j["output_hash"] = output_hash.to_string();
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

  auto hash { HASHER::digest::from_string(json_get(j, "hash")) };

  std::vector<input> inputs;
  for (auto const &j_txi : json_get(j, "inputs"))
    inputs.emplace_back(input::from_json(j_txi));

  std::vector<output> outputs;
  for (auto const &j_txo : json_get(j, "outputs"))
    outputs.emplace_back(output::from_json(j_txo));

  return Transaction { type, index, hash, inputs, outputs };
}

template Transaction<> Transaction<>::from_json(json const &data);

template<typename KEY_PAIR, typename HASHER>
bool
Transaction<KEY_PAIR, HASHER>::valid_reward() const
{
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

template bool Transaction<>::valid_reward() const;

template<typename KEY_PAIR, typename HASHER>
Transaction<KEY_PAIR, HASHER>::digest
Transaction<KEY_PAIR, HASHER>::determine_hash() const
{
  std::stringstream ss;

  ss << m_index;

  for (auto const &txi : m_inputs)
    ss << txi.output_hash.to_string()
       << txi.output_index;

  for (auto const &txo : m_outputs)
    ss << txo.amount
       << txo.address;

  return HASHER::instance().hash(ss.str());
}

template Transaction<>::digest Transaction<>::determine_hash() const;

template<typename KEY_PAIR, typename HASHER>
bool
TransactionGroup<KEY_PAIR, HASHER>::valid(std::size_t index) const
{
  if (m_transactions.size() != config().transaction_num_per_block)
    return false;

  if (m_transactions[0].type() != transaction::Type::REWARD)
    return false;

  for (std::size_t i { 0 }; i < m_transactions.size(); ++i) {
    auto const &t { m_transactions[i] };

    if (t.type() != (i == 0 ? transaction::Type::REWARD : transaction::Type::STANDARD))
      return false;

    if (t.index() != index)
      return false;

    if (!t.valid())
      return false;
  }

  return true;
}

template bool TransactionGroup<>::valid(std::size_t index) const;

template<typename KEY_PAIR, typename HASHER>
json
TransactionGroup<KEY_PAIR, HASHER>::to_json() const
{
  json j = json::array();

  for (auto const &t : m_transactions)
    j.push_back(t.to_json());

  return j;
}

template json TransactionGroup<>::to_json() const;

template<typename KEY_PAIR, typename HASHER>
TransactionGroup<KEY_PAIR, HASHER>
TransactionGroup<KEY_PAIR, HASHER>::from_json(json const &j)
{
  std::vector<transaction> ts;

  for (auto const &j_ : j)
    ts.emplace_back(transaction::from_json(j_));

  return TransactionGroup { ts };
}

template TransactionGroup<> TransactionGroup<>::from_json(json const &data);

template<typename KEY_PAIR, typename HASHER>
void
TransactionGroup<KEY_PAIR, HASHER>::update_unspent_outputs() const
{
  for (auto const &t : m_transactions) {
    auto txohash { t.hash() };
    auto const &txos { t.outputs() };

    for (std::size_t i { 0 }; i < txos.size(); ++i)
      m_unspent_outputs->emplace_back(txohash, i, txos[i]);
  }

  for (auto const &t : m_transactions) {
    for (auto const &txi : t.inputs()) {
      for (auto it { m_unspent_outputs->begin() }; it != m_unspent_outputs->end(); ++it) {

        if (it->output_hash == txi.output_hash &&
            it->output_index == txi.output_index) {

          it = m_unspent_outputs->erase(it);
        }
      }
    }
  }
}

template void TransactionGroup<>::update_unspent_outputs() const;

} // end namespace bc
