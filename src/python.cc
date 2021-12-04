#include <pybind11/chrono.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11_json/pybind11_json.hpp>

#include "blockchain.h"
#include "json.h"

namespace py = pybind11;
using namespace py::literals;

using namespace bm;

PYBIND11_MODULE(bc, m)
{
  py::class_<Block<>>(m, "Block")
    .def("__repr__", &Block<>::to_json)
    .def("data", &Block<>::data)
    .def("timestamp", &Block<>::timestamp)
    .def("index", &Block<>::index)
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
}
