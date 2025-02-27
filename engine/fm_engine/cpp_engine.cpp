// c++ -std=c++17 -O3 -shared -fPIC $(python3 -m pybind11 --includes) fm_engine/cpp_engine.cpp -o fm_engine/cpp_engine$(python3-config --extension-suffix) -I../sdsl/include -L../sdsl/lib -lsdsl -ldivsufsort -ldivsufsort64 -pthread

#include "cpp_engine.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(cpp_engine, m) {

    py::class_<FindResult>(m, "FindResult")
        .def_readwrite("cnt", &FindResult::cnt)
        .def_readwrite("segment_by_shard", &FindResult::segment_by_shard);

    py::class_<CountResult>(m, "CountResult")
        .def_readwrite("count", &CountResult::count);

    py::class_<DocResult>(m, "DocResult")
        .def_readwrite("doc_ix", &DocResult::doc_ix)
        .def_readwrite("doc_len", &DocResult::doc_len)
        .def_readwrite("disp_len", &DocResult::disp_len)
        .def_readwrite("needle_offset", &DocResult::needle_offset)
        .def_readwrite("meta", &DocResult::meta)
        .def_readwrite("data", &DocResult::data);

    py::class_<Engine>(m, "Engine")
        .def(py::init<const vector<string>, const bool, const bool>())
        .def("find", &Engine::find, py::call_guard<py::gil_scoped_release>(), "query"_a)
        .def("count", &Engine::count, py::call_guard<py::gil_scoped_release>(), "query"_a)
        .def("get_doc_by_rank", &Engine::get_doc_by_rank, py::call_guard<py::gil_scoped_release>(), "s"_a, "rank"_a, "needle_len"_a, "max_ctx_len"_a);
}
