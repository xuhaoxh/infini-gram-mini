// c++ -std=c++17 -O3 -shared -fPIC $(python3 -m pybind11 --includes) fm_engine/cpp_engine.cpp -o fm_engine/cpp_engine$(python3-config --extension-suffix) -I/mmfs1/gscratch/h2lab/xuhao/fm_index/sdsl-lite/include -L/mmfs1/gscratch/h2lab/xuhao/fm_index/sdsl-lite/lib -lsdsl -ldivsufsort -ldivsufsort64 -pthread 

#include "cpp_engine.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(cpp_engine, m) {

    py::class_<FMIndexShard>(m, "FMIndexShard")
        .def_readwrite("path", &FMIndexShard::path)
        .def_readwrite("fmIndex", &FMIndexShard::fmIndex);

    py::class_<CountResult>(m, "CountResult")
        .def_readwrite("count", &CountResult::count)
        .def_readwrite("count_by_shard", &CountResult::count_by_shard)
        .def_readwrite("lo_by_shard", &CountResult::lo_by_shard);

    py::class_<LocateResult>(m, "LocateResult")
        .def_readwrite("location", &LocateResult::location)
        .def_readwrite("shard_num", &LocateResult::shard_num);

    py::class_<ReconstructResult>(m, "ReconstructResult")
        .def_readwrite("text", &ReconstructResult::text)
        .def_readwrite("shard_num", &ReconstructResult::shard_num);

    py::class_<Engine>(m, "Engine")
        .def(py::init<const vector<string>>())
        .def("count", &Engine::count, "query"_a)
        .def("locate", &Engine::locate, "query"_a, "num_occ"_a)
        .def("reconstruct", &Engine::reconstruct, "query"_a, "num_occ"_a, "pre_text"_a, "post_text"_a);
}
