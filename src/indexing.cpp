// g++ -std=c++17 -I../sdsl/include -L../sdsl/lib indexing.cpp -o cpp_indexing -lsdsl -ldivsufsort -ldivsufsort64
// GCILK=true g++ -std=c++11 -I../parallel_sdsl/include -L../parallel_sdsl/lib indexing.cpp -o cpp_indexing -lsdsl -ldivsufsort -ldivsufsort64 -DCILKP -fcilkplus

#include <sdsl/suffix_arrays.hpp>
#include <string>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <chrono>

using namespace sdsl;
using namespace std;
using namespace std::chrono;


int construct(string data_dir, string index_dir) {
    string index_file = index_dir + "/data.fm9";
    string meta_index_file = index_dir + "/meta.fm9";
    csa_wt<wt_huff<rrr_vector<127> >, 32, 64> fm_index;
    csa_wt<wt_huff<rrr_vector<127> >, 32, 64> metadata_index;

    if (!load_from_file(fm_index, index_file)) {
        // ifstream in(file_path + ".text");
        // if (!in) {
        //     cerr << "ERROR: Text file " << file_path + ".text" << " does not exist. Exiting." << endl;
        //     return 1;
        // }
        // cout << "Building index for " << file_path << " now." << endl;

        memory_monitor::start();
        sdsl::cache_config config(true, index_dir, "data");
        construct(fm_index, index_dir + "/data", config, 1);
        store_to_file(fm_index, index_file);
        memory_monitor::stop();
    }

    if (!load_from_file(metadata_index, meta_index_file)) {
        // ifstream in(file_path + "_meta");
        // if (!in) {
        //     cerr << "ERROR: Meta file " << file_path + "_meta" << " does not exist. Exiting." << endl;
        //     return 1;
        // }
        // cout << "Building metadata index for " << file_path << " now." << endl;

        memory_monitor::start();
        sdsl::cache_config config(true, index_dir, "meta");
        construct(metadata_index, index_dir + "/meta", config, 1);
        store_to_file(metadata_index, meta_index_file);
        memory_monitor::stop();
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " [directory or file path] [directory to write index]" << endl;
        return 1;
    }

    string data_dir = argv[1];
    string index_directory = argv[2];

    construct(data_dir, index_directory);

    return 0;
}
