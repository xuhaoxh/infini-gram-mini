#include <sdsl/suffix_arrays.hpp>
#include <string>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include "../nlohmann/json.hpp" // TODO: modify path

using namespace sdsl;
using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;
using json = nlohmann::json;

int process(const string index_dir, const string path) {
    memory_monitor::start();
    auto start = high_resolution_clock::now();

    string input_file = path;
    string name = fs::path(path).stem().string();
    string text_file = index_dir + name + ".text";
    string meta_file = index_dir + name + "_meta";
    string offset_file = index_dir + name + "_offset.bin";
    string metaoffset_file = index_dir + name + "_metaoffset.bin";
    const string separator = "\xff";

    ifstream input(input_file);
    ofstream text_output(text_file, ios::app | ios::binary);
    ofstream meta_output(meta_file, ios::app | ios::binary);
    ofstream offset_output(offset_file, ios::app | ios::binary);
    ofstream metaoffset_output(metaoffset_file, ios::app | ios::binary);

    if (!input.is_open() || !text_output.is_open() || !meta_output.is_open() || !offset_output.is_open()) {
        cerr << "Error opening files" << endl;
        return 1;
    }

    string line;
    meta_output << separator;
    while (getline(input, line)) {
        size_t current_text_pos = static_cast<size_t>(text_output.tellp());
        offset_output.write(reinterpret_cast<const char*>(&current_text_pos), sizeof(size_t));

        json entry = json::parse(line);
        string text = " " + separator + " " + entry["text"].get<string>();
        text_output << text;

        size_t current_meta_pos = static_cast<size_t>(meta_output.tellp());
        metaoffset_output.write(reinterpret_cast<const char*>(&current_meta_pos), sizeof(size_t));

        // entry["meta"] for pile, entry["metadata"] for DCLM
        // meta_output << entry["meta"].dump() << separator;
        meta_output << entry["metadata"].dump() << separator;
    }
    size_t final_text_pos = static_cast<size_t>(text_output.tellp());
    offset_output.write(reinterpret_cast<const char*>(&final_text_pos), sizeof(size_t));

    size_t final_meta_pos = static_cast<size_t>(meta_output.tellp());
    metaoffset_output.write(reinterpret_cast<const char*>(&final_meta_pos), sizeof(size_t));

    auto end = high_resolution_clock::now();
    memory_monitor::stop();

    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Time taken for processing text: " << duration.count() / 1000.00 << " s" << endl;

    return 0;
}

int construct(string index_dir, string path) {
    string file_path = index_dir + fs::path(path).stem().string();
    string index_file = file_path + ".fm9";
    string meta_index_file = file_path + "_meta.fm";
    csa_wt<wt_huff<rrr_vector<127> >, 32, 64> fm_index;
    csa_wt<wt_huff<rrr_vector<127> >, 32, 64> metadata_index;

    if (!load_from_file(fm_index, index_file)) {
        ifstream in(file_path + ".text");
        if (!in) {
            cerr << "ERROR: Text file " << file_path + ".text" << " does not exist. Exiting." << endl;
            return 1;
        }
        cout << "Building index for " << file_path << " now." << endl;

        memory_monitor::start();
        auto start = high_resolution_clock::now();
        sdsl::cache_config config(true, index_dir);
        construct(fm_index, file_path + ".text", config, 1);
        auto end = high_resolution_clock::now();
        memory_monitor::stop();

        auto duration = duration_cast<milliseconds>(end - start);
        cout << "Index construction done after " << duration.count() / 1000.00 << " s" << endl;
        store_to_file(fm_index, index_file);
    }

    if (!load_from_file(metadata_index, meta_index_file)) {
        ifstream in(file_path + "_meta");
        if (!in) {
            cerr << "ERROR: Meta file " << file_path + "_meta" << " does not exist. Exiting." << endl;
            return 1;
        }
        cout << "Building metadata index for " << file_path << " now." << endl;

        memory_monitor::start();
        auto meta_start = high_resolution_clock::now();
        sdsl::cache_config config(true, index_dir);
        construct(metadata_index, file_path + "_meta", config, 1);
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

    string path = argv[1];
    if (!fs::exists(path)) {
        cerr << "ERROR: Path " << path << " does not exist. Exiting." << endl;
        return 1;
    }

    string index_directory = argv[2];
    if (!fs::exists(index_directory)) {
        cout << "Creating indexes folder..." << endl;
        fs::create_directories(index_directory);
    }

    if (fs::is_directory(path)) {
        string dir = index_directory + fs::path(path).filename().string();
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
        }
        for (auto entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                string entry_dir = dir + "/" + entry.path().stem().string() + "/";
                if (!fs::exists(entry_dir)) {
                    fs::create_directory(entry_dir);
                }

                if (process(entry_dir, entry.path().string()) == 1) {
                    return 1;
                }
                if (construct(entry_dir, entry.path().string()) == 1) {
                    return 1;
                }
            }
        }
    } else if (fs::is_regular_file(path)) {
        string dir = index_directory + fs::path(path).stem().string() + "/";
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
        }
        if (process(dir, path) == 1) {
            return 1;
        }
        if (construct(dir, path) == 1) {
            return 1;
        }
    } else {
        cerr << "ERROR: The provided path should be a directory or a regular file. Exiting." << endl;
        return 1;
    }

    return 0;
}
