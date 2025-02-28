// g++ -std=c++17 -I../sdsl/include -L../sdsl/lib indexing.cpp -o cpp_indexing -lsdsl -ldivsufsort -ldivsufsort64
// GCILK=true g++ -std=c++11 -I../parallel_sdsl/include -L../parallel_sdsl/lib indexing.cpp -o cpp_indexing -lsdsl -ldivsufsort -ldivsufsort64 -DCILKP -fcilkplus -O2

#include <sdsl/suffix_arrays.hpp>
#include <string>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <chrono>

using namespace sdsl;
using namespace std;
using namespace std::chrono;


int construct(string index_dir) {
    string index_file = index_dir + "/data.fm9";
    string meta_index_file = index_dir + "/meta.fm9";
    csa_wt<wt_huff<rrr_vector<127> >, 32, 64> fm_index;
    csa_wt<wt_huff<rrr_vector<127> >, 32, 64> metadata_index;

    if (!load_from_file(fm_index, index_file)) {
        memory_monitor::start();
        sdsl::cache_config config(true, index_dir, "data");
        construct(fm_index, index_dir + "/data", config, 1);
        store_to_file(fm_index, index_file);
        memory_monitor::stop();

        std::ofstream fout(index_dir + "/memory_traces.html");
        memory_monitor::write_memory_log<HTML_FORMAT>(fout);
        fout.close();
    }

    if (!load_from_file(metadata_index, meta_index_file)) {
        memory_monitor::start();
        sdsl::cache_config config(true, index_dir, "meta");
        construct(metadata_index, index_dir + "/meta", config, 1);
        store_to_file(metadata_index, meta_index_file);
        memory_monitor::stop();
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " [directory to write index]" << endl;
        return 1;
    }

    string index_directory = argv[1];

    construct(index_directory);

    return 0;
}
