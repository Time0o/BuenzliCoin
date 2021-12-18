#include <pybind11/chrono.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11_json/pybind11_json.hpp>

#include "blockchain.h"
#include "config.h"
#include "json.h"

namespace py = pybind11;
using namespace py::literals;

using namespace bc;

PYBIND11_MODULE(bc, m)
{
  py::class_<Block<>>(m, "Block")
    .def("__repr__", &Block<>::to_json)
    .def("data", &Block<>::data)
    .def("timestamp", &Block<>::timestamp)
    .def("index", &Block<>::index)
    .def("hash", [](Block<> const &self){ return self.hash().to_string(); })
    .def("valid", &Block<>::valid)
    .def("is_genesis", &Block<>::is_genesis)
    .def("is_successor_of", &Block<>::is_successor_of, "prev"_a)
    .def("to_json", &Block<>::to_json)
    .def_static("from_json", &Block<>::from_json, "j"_a);

  py::class_<Blockchain<>>(m, "Blockchain")
    .def("__repr__", &Blockchain<>::to_json)
    .def("__len__", &Blockchain<>::length)
    .def("all_blocks", &Blockchain<>::all_blocks)
    .def("latest_block", &Blockchain<>::latest_block)
    .def("empty", &Blockchain<>::empty)
    .def("length", &Blockchain<>::length)
    .def("valid", &Blockchain<>::valid)
    .def("to_json", &Blockchain<>::to_json)
    .def_static("from_json", &Blockchain<>::from_json, "j"_a);

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
                   &Config::block_gen_difficulty_adjust_factor_limit);

   m.def("config", &config, py::return_value_policy::reference);
}
