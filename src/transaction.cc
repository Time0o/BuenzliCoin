#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "format.h"
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
Transaction<KEY_PAIR, HASHER>
Transaction<KEY_PAIR, HASHER>::reward(std::string const &reward_address,
                                      std::size_t index)
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

template Transaction<> Transaction<>::reward(std::string const &reward_address,
                                             std::size_t index);

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

  j["inputs"] = json::array();
  for (auto const &txi : m_inputs)
    j["inputs"].push_back(txi.to_json());

  j["outputs"] = json::array();
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
std::pair<bool, std::string>
Transaction<KEY_PAIR, HASHER>::valid_standard() const
{
  if (m_hash != determine_hash())
    return { false, "invalid hash" };

  std::size_t txi_sum { 0 };

  for (std::size_t i { 0 }; i < m_inputs.size(); ++i) {
    auto const &txi { m_inputs[i] };

    std::string txi_address;

    for (auto const &utxo : m_unspent_outputs) {
      if (utxo.output_hash == txi.output_hash &&
          utxo.output_index == txi.output_index) {

        txi_address = utxo.output.address;
        txi_sum += utxo.output.amount;
      }
    }

    if (txi_address.empty())
      return { false, fmt::format("input {}: no corresponding unspent output found", i) };

    try {
      typename KEY_PAIR::public_key key { txi_address };

      if (!key.verify(m_hash.to_string(), txi.signature))
        return { false, fmt::format("input {}: invalid signature", i) };

    } catch (std::exception const &e) {
      return { false, fmt::format("input {}: exception during signature validation: {}", i, e.what()) };
    }
  }

  std::size_t txo_sum { 0 };

  for (auto const &txo : m_outputs)
    txo_sum += txo.amount;

  if (txi_sum != txo_sum)
    return { false, fmt::format("mismatched input/output sums") };

  return { true, "" };
}

template std::pair<bool, std::string> Transaction<>::valid_standard() const;

template<typename KEY_PAIR, typename HASHER>
std::pair<bool, std::string>
Transaction<KEY_PAIR, HASHER>::valid_reward() const
{
  if (m_hash != determine_hash())
    return { false, "invalid hash" };

  if (!m_inputs.empty())
    return { false, "inputs must be empty" };

  if (m_outputs.size() != 1)
    return { false, "more than one output" };

  if (m_outputs[0].amount != config().transaction_reward_amount)
    return { false, "output amount does not match reward amount" };

  return { true, "" };
}

template std::pair<bool, std::string> Transaction<>::valid_reward() const;

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
std::pair<bool, std::string>
TransactionList<KEY_PAIR, HASHER>::valid(std::size_t index) const
{
  if (m_transactions.size() > config().transaction_num_per_block + 1)
    return { false, "invalid number of transactions" };

  for (std::size_t i { 0 }; i < m_transactions.size(); ++i) {
    auto const &t { m_transactions[i] };

    if (t.type() != (i == 0 ? transaction::Type::REWARD : transaction::Type::STANDARD))
      return { false, fmt::format("transaction {}: invalid type", i) };

    if (t.index() != index)
      return { false, fmt::format("transaction {}: invalid index {}", i) };

    auto [valid, error] = t.valid();

    if (!valid)
      return { false, fmt::format("transaction {}: {}", i, error) };
  }

  return { true, "" };
}

template std::pair<bool, std::string> TransactionList<>::valid(std::size_t index) const;

template<typename KEY_PAIR, typename HASHER>
json
TransactionList<KEY_PAIR, HASHER>::to_json() const
{
  json j = json::array();

  for (auto const &t : m_transactions)
    j.push_back(t.to_json());

  return j;
}

template json TransactionList<>::to_json() const;

template<typename KEY_PAIR, typename HASHER>
TransactionList<KEY_PAIR, HASHER>
TransactionList<KEY_PAIR, HASHER>::from_json(json const &j)
{
  std::vector<transaction> ts;

  for (auto const &j_ : j)
    ts.emplace_back(transaction::from_json(j_));

  return TransactionList { ts.begin(), ts.end() };
}

template TransactionList<> TransactionList<>::from_json(json const &data);

template<typename KEY_PAIR, typename HASHER>
void
TransactionUnspentOutputs<KEY_PAIR, HASHER>::update(transaction const &t)
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

template void TransactionUnspentOutputs<>::update(transaction const &t);

template<typename KEY_PAIR, typename HASHER>
json
TransactionUnspentOutputs<KEY_PAIR, HASHER>::to_json() const
{
  json j = json::array();
  for (auto const &utxo : m_unspent_outputs)
    j.push_back(utxo.to_json());

  return j;
}

template json TransactionUnspentOutputs<>::to_json() const;

template<typename KEY_PAIR, typename HASHER>
Transaction<KEY_PAIR, HASHER>
TransactionUnconfirmedPool<KEY_PAIR, HASHER>::next()
{
  auto t { m_transactions.front() };

  m_transactions.pop_front();

  return t;
}

template Transaction<> TransactionUnconfirmedPool<>::next();

template<typename KEY_PAIR, typename HASHER>
void
TransactionUnconfirmedPool<KEY_PAIR, HASHER>::add(transaction const &t)
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

template void TransactionUnconfirmedPool<>::add(transaction const &t);

template<typename KEY_PAIR, typename HASHER>
void
TransactionUnconfirmedPool<KEY_PAIR, HASHER>::remove(transaction const &t)
{
  for (auto it { m_transactions.begin() }; it != m_transactions.end(); ++it) {
    if (it->hash() == t.hash())
      it = m_transactions.erase(it);
  }
}

template void TransactionUnconfirmedPool<>::remove(transaction const &t);

template<typename KEY_PAIR, typename HASHER>
void
TransactionUnconfirmedPool<KEY_PAIR, HASHER>::prune(
  std::list<typename transaction::unspent_output> const &unspent_outputs)
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

template void TransactionUnconfirmedPool<>::prune(
  std::list<typename transaction::unspent_output> const &unspent_outputs);

template<typename KEY_PAIR, typename HASHER>
json
TransactionUnconfirmedPool<KEY_PAIR, HASHER>::to_json() const
{
  json j = json::array();
  for (auto const &t : m_transactions)
    j.push_back(t.to_json());

  return j;
}

template json TransactionUnconfirmedPool<>::to_json() const;

} // end namespace bc
