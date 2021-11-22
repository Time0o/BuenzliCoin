#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "blockchain.h"

namespace py = pybind11;
using namespace py::literals;

using namespace bm;

PYBIND11_MODULE(bc, m)
{
  py::class_<Block<>>(m, "Block")
    .def("__repr__", &Block<>::to_json)
    .def("valid", &Block<>::valid)
    .def("to_json", &Block<>::to_json)
    .def_static("from_json", &Block<>::from_json, "j"_a);

  py::class_<Blockchain<>>(m, "Blockchain")
    .def("__repr__", &Blockchain<>::to_json)
    .def("__iter__",
         [](Blockchain<> const &blockchain)
         { return py::make_iterator(blockchain.begin(), blockchain.end()); })
    .def("longer_than", &Blockchain<>::longer_than)
    .def("empty", &Blockchain<>::empty)
    .def("valid", &Blockchain<>::valid)
    .def("to_json", &Blockchain<>::to_json)
    .def_static("from_json", &Blockchain<>::from_json, "j"_a);
}
