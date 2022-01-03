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
void
Transaction<KEY_PAIR, HASHER>::link()
{
  update_unspent_outputs();
}

template void Transaction<>::link();

template<typename KEY_PAIR, typename HASHER>
void
Transaction<KEY_PAIR, HASHER>::link(Transaction const &last)
{
  m_unspent_outputs_last = last.unspent_outputs();
  m_unspent_outputs = m_unspent_outputs;

  update_unspent_outputs();
}

template void Transaction<>::link(Transaction const &last);

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
std::pair<bool, std::string>
Transaction<KEY_PAIR, HASHER>::valid_standard() const
{
  if (m_hash != determine_hash())
    return { false, "invalid hash" };

  std::size_t txi_sum { 0 };

  for (std::size_t i { 0 }; i < m_inputs.size(); ++i) {
    auto const &txi { m_inputs[i] };

    std::string txi_address;

    for (auto const &utxo : m_unspent_outputs_last) {
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
void
Transaction<KEY_PAIR, HASHER>::update_unspent_outputs()
{
  for (std::size_t i { 0 }; i < m_outputs.size(); ++i)
    m_unspent_outputs.emplace_back(m_hash, i, m_outputs[i]);

  for (auto const &txi : m_inputs) {
    for (auto it { m_unspent_outputs.begin() }; it != m_unspent_outputs.end(); ++it) {
      if (it->output_hash == txi.output_hash &&
          it->output_index == txi.output_index) {

        it = m_unspent_outputs.erase(it);
      }
    }
  }
}

template void Transaction<>::update_unspent_outputs();

template<typename KEY_PAIR, typename HASHER>
std::pair<bool, std::string>
TransactionGroup<KEY_PAIR, HASHER>::valid(std::size_t index) const
{
  if (m_transactions.size() != config().transaction_num_per_block)
    return { false, "invalid number of transactions" };

  for (std::size_t i { 0 }; i < m_transactions.size(); ++i) {
    auto const &t { m_transactions[i] };

    if (t.type() != (i == 0 ? transaction::Type::REWARD : transaction::Type::STANDARD))
      return { false, fmt::format("transaction {}: invalid type", i) };

    if (t.index() != index)
      return { false, fmt::format("transaction {}: invalid index", i) };

    auto [valid, error] = t.valid();

    if (!valid)
      return { false, fmt::format("transaction {}: {}", i, error) };
  }

  return { true, "" };
}

template std::pair<bool, std::string> TransactionGroup<>::valid(std::size_t index) const;

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
TransactionGroup<KEY_PAIR, HASHER>::link()
{
  m_transactions[0].link();

  for (std::size_t i { 1 }; i < m_transactions.size(); ++i)
    m_transactions[i].link(m_transactions[i - 1]);
}

template void TransactionGroup<>::link();

template<typename KEY_PAIR, typename HASHER>
void
TransactionGroup<KEY_PAIR, HASHER>::link(TransactionGroup const &last)
{
  m_transactions[0].link(last.transactions().back());

  for (std::size_t i { 1 }; i < m_transactions.size(); ++i)
    m_transactions[i].link(m_transactions[i - 1]);
}

template void TransactionGroup<>::link(TransactionGroup const &last);

} // end namespace bc
