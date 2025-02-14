#include <sdsl/suffix_arrays.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <filesystem>
#include <thread>
#include <mutex>
#include <numeric>
#include <chrono>
#include <sys/stat.h> 
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <typeinfo>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;
using namespace sdsl;
namespace fs = std::filesystem;
using namespace chrono;

typedef unsigned long size_t;
typedef unsigned long char_t;

// typedef csa_wt<wt_huff<rrr_vector<127>>, 512, 1024> index_t;
typedef csa_wt<wt_huff<rrr_vector<127>>, 32, 64> index_t;
typedef csa_wt<wt_huff<rrr_vector<127>>, 32, 64> meta_index_t;

struct FMIndexShard {
    string path;
    index_t* fmIndex;
    size_t size;
    size_t* offset;
    size_t* meta_offset;
    size_t num_offsets;
    meta_index_t* metadata;
};

struct CountResult {
    size_t count;
    vector<size_t> count_by_shard;
    vector<size_t> lo_by_shard;
};

struct LocateResult {
    size_t location;
    size_t shard_num;
};

struct ReconstructResult {
    string text;
    size_t shard_num;
    string metadata;
};

class Engine {
public:
    Engine (const vector<string> index_dirs, bool load_to_ram, bool get_metadata) 
            : _load_to_ram(load_to_ram), _get_metadata(get_metadata) {
        vector<thread> threads;

        for (const auto &index_dir : index_dirs) {
            assert (fs::exists(index_dir));

            vector<string> id_paths;
            
            for (const auto &entry : fs::recursive_directory_iterator(index_dir)) {
                if (entry.path().extension() == ".fm9") {
                    id_paths.push_back(entry.path().string());
                }
            }
            sort(id_paths.begin(), id_paths.end());

            auto start_time = high_resolution_clock::now();

            for (size_t s = 0; s < id_paths.size(); s++) {
                threads.emplace_back([this, id_path = id_paths[s]]() {
                    size_t* offset;
                    size_t* meta_offset;
                    size_t num_offsets;
                    meta_index_t* metadata;
                    if (_get_metadata) {
                        std::tie(offset, meta_offset, num_offsets, metadata) = load_meta(id_path.substr(0, id_path.length() - 4));
                    }
                    auto [fm_index, index_size] = load_file(id_path);
                    auto shard = FMIndexShard{id_path, fm_index, index_size, offset, meta_offset, num_offsets, metadata};

                    lock_guard<mutex> lock(mtx);
                    _shards.push_back(shard);
                });
            }

            for (auto& thread : threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }

            auto end_time = high_resolution_clock::now();
            auto load_time = duration_cast<milliseconds>(end_time - start_time).count();
            cout << "Load time: " << load_time / 1000.00 << " s." << endl;

            _num_shards = _shards.size();
            assert(_num_shards > 0);
        }
    }

    ~Engine() {
        for (auto& shard : _shards) {
            // delete shard.fmIndex;
            if (_load_to_ram) {
                delete shard.fmIndex;
            } else {
                munmap(shard.fmIndex, shard.size);
            }
        }
    }

    tuple<size_t*, size_t*, size_t, meta_index_t*> load_meta(const string& file) {
        // mmap file offset
        string offset_file = file + "_offset.bin";
        int fd = open(offset_file.c_str(), O_RDONLY);
        assert(fd >= 0);
    
        off_t offset_size = lseek(fd, 0, SEEK_END);
        assert(offset_size > 0);

        size_t* mapped_offset = (size_t*)mmap(nullptr, offset_size, PROT_READ, MAP_PRIVATE, fd, 0);
        assert(mapped_offset != MAP_FAILED);

        size_t num_offsets = offset_size / sizeof(size_t);

        // mmap metadata offset
        string meta_offset_file = file + "_metaoffset.bin";
        int meta_fd = open(meta_offset_file.c_str(), O_RDONLY);
        assert(meta_fd >= 0);
    
        off_t meta_offset_size = lseek(meta_fd, 0, SEEK_END);
        assert(meta_offset_size > 0);

        size_t* mapped_meta_offset = (size_t*)mmap(nullptr, meta_offset_size, PROT_READ, MAP_PRIVATE, meta_fd, 0);
        assert(mapped_meta_offset != MAP_FAILED);

        // mmap metadata index
        string meta_path = file + "_meta.fm";
        auto meta_index = new meta_index_t();

        if (_load_to_ram) {
            load_from_file(*meta_index, meta_path);
        } else {
            load_from_file_(*meta_index, meta_path);
        }

        return {mapped_offset, mapped_meta_offset, num_offsets, meta_index};
    }

    tuple<vector<size_t>, vector<size_t>, meta_index_t*> load_meta_(const string& file) {
        ifstream offset_file(file + ".offset");
        vector<size_t> offset;
        assert(offset_file.is_open());
        string line;
        while (getline(offset_file, line)) {
            stringstream ss(line);
            string item;
            while (getline(ss, item, ',')) {
                offset.push_back(stoul(item));
            }
        }
        offset_file.close();

        ifstream meta_offset_file(file + ".metaoffset");
        vector<size_t> meta_offset;
        assert(meta_offset_file.is_open());
        string meta_line;
        while (getline(meta_offset_file, meta_line)) {
            stringstream meta_ss(meta_line);
            string meta_item;
            while (getline(meta_ss, meta_item, ',')) {
                meta_offset.push_back(stoul(meta_item));
            }
        }
        meta_offset_file.close();

        string meta_path = file + "_meta.fm";
        auto meta_index = new meta_index_t();

        if (_load_to_ram) {
            load_from_file(*meta_index, meta_path);
        } else {
            load_from_file_(*meta_index, meta_path);
        }

        return {offset, meta_offset, meta_index};
    }

    pair<index_t*, size_t> load_file(const string& path) {
        if (_load_to_ram) {
            auto fm_index = new index_t();
            load_from_file(*fm_index, path);
            return {fm_index, fm_index->size()};
        } else {
            auto fm_index = new index_t();
            load_from_file_(*fm_index, path);
            return {fm_index, fm_index->size()};
        }
    }


    CountResult count(const string& query) {
        vector<size_t> count_by_shards(_num_shards, 0);
        vector<size_t> lo_by_shards(_num_shards, 0);
        vector<thread> threads;

        if (query.length() == 0) {
            return {0, count_by_shards, lo_by_shards};
        }

        for (size_t i = 0; i < _num_shards; ++i) {
            threads.emplace_back(&Engine::_count, this, _shards[i], query, &count_by_shards[i], &lo_by_shards[i]);
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        size_t total_cnt = accumulate(count_by_shards.begin(), count_by_shards.end(), 0);
        return CountResult{total_cnt, count_by_shards, lo_by_shards};
    }

    void _count(FMIndexShard fm_index, const string& query, size_t* out_count, size_t* out_lo) {
        size_t lo = 0;
        size_t hi = 0;
        
        auto counts = sdsl::backward_search(*fm_index.fmIndex, 0, fm_index.size - 1, query.begin(), query.end(), lo, hi);
        *out_count = counts;
        *out_lo = lo;
    }

    LocateResult locate(const string& query, size_t num_occ) {
        auto count_result = count(query);
        auto counts = count_result.count;
        auto count_by_shard = count_result.count_by_shard;
        auto lo_by_shard = count_result.lo_by_shard;

        if (counts == 0) {
            return {SIZE_MAX, SIZE_MAX};
        }

        if (num_occ > counts) {
            throw runtime_error("num_occ is larger than the number of occurrences.");
        }

        size_t shard_num;
        size_t num_occ_prev = 0;
        size_t offset;
        for (shard_num = 0; shard_num < _num_shards; shard_num++) {
            if (num_occ_prev + count_by_shard[shard_num] >= num_occ) break;
            num_occ_prev += count_by_shard[shard_num];
        }
        offset = num_occ - num_occ_prev;

        size_t location = (*_shards[shard_num].fmIndex)[lo_by_shard[shard_num] + offset - 1]; 
        // if (temp_path != _shards[shard_num].path) {
        //     temp_path = _shards[shard_num].path;
        //     load_from_file(temp_index, temp_path);
        // }
        // size_t location = temp_index[lo_by_shard[shard_num] + offset - 1]; 
        return LocateResult{location, shard_num};
    }

    ReconstructResult reconstruct(const string& query, size_t num_occ, size_t pre_text=400, size_t post_text=400) {
        auto locate_result = locate(query, num_occ);
        size_t location = locate_result.location;
        size_t shard_num = locate_result.shard_num;

        if (location == SIZE_MAX) {
            return {"", SIZE_MAX};
        }

        size_t start = (location < pre_text) ? 0 : location - pre_text;
        size_t end = std::min(location + query.length() + post_text, _shards[shard_num].size - 1);
        
        auto s = sdsl::extract(*_shards[shard_num].fmIndex, start, end);
        // auto s = sdsl::extract(temp_index, location - pre_text, location + query.length() + post_text);
        auto pos = s.find("\xff");
        while (pos != std::string::npos) {
            if (pos > pre_text) {
                s.erase(pos);
            } else {
                s.erase(0, pos+2);
                pre_text -= pos + 2;
            }
            pos = s.find("\xff");
        }

        if (_get_metadata) {
            string metadata = get_metadata(location, shard_num);
            return ReconstructResult{s, shard_num, metadata};
        } else {
            return ReconstructResult{s, shard_num, ""};
        }
        
    }

    string get_metadata(size_t location, size_t shard_num) {
        size_t* offset = _shards[shard_num].offset;
        size_t* meta_offset = _shards[shard_num].meta_offset;
        
        // TODO: speed up using binary search
        size_t idx = 0;
        while (*(offset + idx) <= location) {
            idx++;
        }

        if (idx > _shards[shard_num].num_offsets) {
            return "";
        }
        
        size_t lower_pos = *(meta_offset + idx - 1);
        size_t upper_pos = *(meta_offset + idx);
        
        string meta = sdsl::extract(*_shards[shard_num].metadata, lower_pos, upper_pos);
        return meta.substr(0, meta.length()-2);
    }

    // string get_metadata_(size_t location, size_t shard_num) {
    //     auto it = upper_bound(_shards[shard_num].offset.begin(), _shards[shard_num].offset.end(), location);
    //     if (it == _shards[shard_num].offset.end()) {
    //         return "";
    //     }
    //     size_t idx = distance(_shards[shard_num].offset.begin(), it);
        
    //     size_t lower_pos = _shards[shard_num].meta_offset[idx-1];
    //     size_t upper_pos = _shards[shard_num].meta_offset[idx];
    //     // cout << "lower pos: " << lower_pos << ", upper pos: " << upper_pos << endl;
    //     // cout << "extract length: " << upper_pos - lower_pos << endl;
    //     string meta = sdsl::extract(*_shards[shard_num].metadata, lower_pos, upper_pos);
    //     return meta.substr(0, meta.length()-2);
    // }

private:
    vector<FMIndexShard> _shards;
    size_t _num_shards;
    bool _load_to_ram;
    bool _get_metadata;
    mutex mtx;
};
