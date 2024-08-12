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

using namespace std;
using namespace sdsl;
namespace fs = std::filesystem;

typedef unsigned long size_t;
typedef unsigned long char_t;

typedef csa_wt<wt_huff<rrr_vector<127>>, 512, 1024> index_t;

struct FMIndexShard {
    string path;
    index_t* fmIndex;
    size_t size;
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
};

class Engine {
public:
    Engine (const vector<string> index_dirs) {
        vector<thread> threads;

        for (const auto &index_dir : index_dirs) {
            assert (fs::exists(index_dir));

            vector<string> id_paths;
            for (const auto &entry : fs::directory_iterator(index_dir)) {
                if (entry.path().extension() == ".fm9") {
                    id_paths.push_back(entry.path().string());
                }
            }
            sort(id_paths.begin(), id_paths.end());

            for (size_t s = 0; s < id_paths.size(); s++) {
                threads.emplace_back([this, id_path = id_paths[s]]() {
                    // auto [fm_index, index_size] = load_file(id_path);
                    // auto shard = FMIndexShard{id_path, fm_index, index_size};

                    auto fm_index = new csa_wt<wt_huff<rrr_vector<127>>, 512, 1024>();
                    load_from_file(*fm_index, id_path);
                    auto shard = FMIndexShard{id_path, fm_index, fm_index->size()};
                    
                    lock_guard<mutex> lock(mtx);
                    _shards.push_back(shard);
                });
            }

            for (auto& thread : threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }

            _num_shards = _shards.size();
            assert(_num_shards > 0);
        }
    }

    ~Engine() {
        for (auto& shard : _shards) {
            delete shard.fmIndex;
            // if (_load_to_ram) {
            //     delete shard.fmIndex;
            // } else {
            //     munmap(shard.fmIndex, shard.size);
            // }
        }
    }

    // pair<index_t*, size_t> load_file(const string& path) {
    //     if (_load_to_ram) {
    //         auto fm_index = new index_t();
    //         load_from_file(*fm_index, path);
    //         return {fm_index, fm_index->size()};
    //     } else {
    //         int f = open(path.c_str(), O_RDONLY);
    //         assert (f != -1);
    //         struct stat s;
    //         assert (fstat(f, &s) != -1);
    //         index_t* ptr = reinterpret_cast<index_t*>(mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, f, 0));
    //         assert (ptr != MAP_FAILED);
    //         madvise(ptr, s.st_size, MADV_RANDOM);
    //         close(f);

    //         auto temp = new index_t();
    //         load_from_file(*temp, path);

    //         assert (temp->size() == ptr->size());
    //         assert (typeid(*temp).name() == typeid(*ptr).name());

    //         return {ptr, s.st_size};
    //     }
    // }

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

        // auto index = *fm_index.fmIndex;
        // cout << fm_index.fmIndex->char2comp.size() << endl;
        
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
            cout << "num_occ is larger than number of occcurrences, locating/reconstructing the last occurrence..." << endl;
            num_occ = counts;
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

        return LocateResult{location, shard_num};
    }

    // LocateResult locate_given_count(const string& query, size_t num_occ, vector<size_t> count_by_shards) {
    //     size_t shard_num;
    //     size_t num_occ_prev = 0;
    //     size_t offset;
    //     for (shard_num = 0; shard_num < _num_shards; shard_num++) {
    //         if (num_occ_prev + count_by_shards[shard_num] > num_occ) break;
    //         num_occ_prev += count_by_shards[shard_num];
    //     }
    //     offset = num_occ - num_occ_prev;
    //     size_t location = _locate(_shards[shard_num].fmIndex, query, offset);
    //     return LocateResult{location, shard_num};
    // }

    // size_t _locate (index_t* fm_index, const string& query, size_t num_locate) {
    //     size_t lo = 0;
    //     size_t hi = 0;
    //     auto num_occ = sdsl::backward_search(*fm_index, 0, fm_index->size() - 1, query.begin(), query.end(), lo, hi);
    //     size_t location = (*fm_index)[lo + num_locate - 1];
    //     return location;
    // }

    ReconstructResult reconstruct(const string& query, size_t num_occ, size_t pre_text=400, size_t post_text=400) {
        auto locate_result = locate(query, num_occ);
        size_t location = locate_result.location;
        size_t shard_num = locate_result.shard_num;

        if (location == SIZE_MAX) {
            return {"", SIZE_MAX};
        }

        auto s = sdsl::extract(*_shards[shard_num].fmIndex, location - pre_text, location + query.length() + post_text);
        return ReconstructResult{string(s.begin(), s.end()), shard_num};
    }

    // ReconstructResult _reconstruct(size_t location, size_t shard_number, size_t query_length, size_t pre_text=400, size_t post_text=400) {
    //     auto s = sdsl::extract(*_shards[shard_number].fmIndex, location - pre_text, location + query_length + post_text);
    //     return ReconstructResult{string(s.begin(), s.end()), shard_number};
    // }
    

private:
    vector<FMIndexShard> _shards;
    size_t _num_shards;
    // bool _load_to_ram;
    mutex mtx;
};
