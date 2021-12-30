#include <pybind11/chrono.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11_json/pybind11_json.hpp>

#include "blockchain.h"
#include "config.h"
#include "json.h"
#include "text.h"
#include "transaction.h"

namespace py = pybind11;
using namespace py::literals;

using namespace bc;

PYBIND11_MODULE(bc, m)
{
#ifdef TRANSACTION
  using block = Block<Transaction>;
  using blockchain = Blockchain<Transaction>;
#else
  using block = Block<Text>;
  using blockchain = Blockchain<Text>;
#endif // TRANSACTION

  py::class_<block>(m, "Block")
    .def("__repr__", &block::to_json)
    .def("data", [](block const &b){ return b.data().to_json(); })
    .def("timestamp", &block::timestamp)
    .def("index", &block::index)
    .def("hash", [](block const &self){ return self.hash().to_string(); })
    .def("valid", &block::valid)
    .def("is_genesis", &block::is_genesis)
    .def("is_successor_of", &block::is_successor_of, "prev"_a)
    .def("to_json", &block::to_json)
    .def_static("from_json", &block::from_json, "j"_a);

  py::class_<blockchain>(m, "Blockchain")
    .def("__repr__", &blockchain::to_json)
    .def("__len__", &blockchain::length)
    .def("all_blocks", &blockchain::all_blocks)
    .def("latest_block", &blockchain::latest_block)
    .def("empty", &blockchain::empty)
    .def("length", &blockchain::length)
    .def("valid", &blockchain::valid)
    .def("to_json", &blockchain::to_json)
    .def_static("from_json", &blockchain::from_json, "j"_a);

  py::class_<Config>(m, "Config")
    .def_readwrite("block_gen_time_expected",
                   &Config::block_gen_time_expected)
    .def_readwrite("block_gen_time_max_delta",
                   &Config::block_gen_time_max_delta)
    .def_readwrite("block_gen_difficulty_init",
                   &Config::block_gen_difficulty_init)
    .def_readwrite("block_gen_difficulty_adjust_after",
                   &Config::block_gen_difficulty_adjust_after)
    .def_readwrite("block_gen_difficulty_adjust_factor_limit",
                   &Config::block_gen_difficulty_adjust_factor_limit)
    .def_readwrite("transaction_reward_amount",
                   &Config::transaction_reward_amount);

   m.def("config", &config, py::return_value_policy::reference);

  py::class_<ECSecp256k1PrivateKey>(m, "ECSecp256k1PrivateKey")
    .def(py::init<std::string_view>())
    .def("sign",
         [](ECSecp256k1PrivateKey const &key, std::string_view msg)
         {
           return key.sign(msg).to_string();
         });

  py::class_<ECSecp256k1PublicKey>(m, "ECSecp256k1PublicKey")
    .def(py::init<std::string_view>())
    .def("verify",
         [](ECSecp256k1PublicKey const &key,
            std::string_view msg,
            std::string const &signature)
         {
            auto digest { ECSecp256k1PublicKey::digest::from_string(signature) };

            return key.verify(msg, digest);
         });
}
