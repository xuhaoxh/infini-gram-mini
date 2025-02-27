// g++ -std=c++17 -I../sdsl/include -L../sdsl/lib indexing.cpp -o cpp_indexing -lsdsl -ldivsufsort -ldivsufsort64

#include <sdsl/suffix_arrays.hpp>
#include <string>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <filesystem>

using namespace sdsl;
using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;


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
        auto start = high_resolution_clock::now();
        sdsl::cache_config config(true, index_dir, "data");
        construct(fm_index, index_dir + "/data", config, 1);
        auto end = high_resolution_clock::now();
        memory_monitor::stop();

        auto duration = duration_cast<milliseconds>(end - start);
        cout << "Index construction done after " << duration.count() / 1000.00 << " s" << endl;
        store_to_file(fm_index, index_file);
    }

    if (!load_from_file(metadata_index, meta_index_file)) {
        // ifstream in(file_path + "_meta");
        // if (!in) {
        //     cerr << "ERROR: Meta file " << file_path + "_meta" << " does not exist. Exiting." << endl;
        //     return 1;
        // }
        // cout << "Building metadata index for " << file_path << " now." << endl;

        memory_monitor::start();
        auto meta_start = high_resolution_clock::now();
        sdsl::cache_config config(true, index_dir, "meta");
        construct(metadata_index, index_dir + "/meta", config, 1);
        store_to_file(metadata_index, meta_index_file);
        auto meta_end = high_resolution_clock::now();
        memory_monitor::stop();

        auto meta_duration = duration_cast<milliseconds>(meta_end - meta_start);
        cout << "Metadata index construction done after " << meta_duration.count() / 1000.00 << " s" << endl;
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " [directory or file path] [directory to write index]" << endl;
        return 1;
    }

    string data_dir = argv[1];
    if (!fs::exists(data_dir)) {
        cerr << "ERROR: Path " << data_dir << " does not exist. Exiting." << endl;
        return 1;
    }

    string index_directory = argv[2];
    if (!fs::exists(index_directory)) {
        cout << "Creating indexes folder..." << endl;
        fs::create_directories(index_directory);
    }

    construct(data_dir, index_directory);

    return 0;
}
