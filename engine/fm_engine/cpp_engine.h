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

typedef csa_wt<wt_huff<rrr_vector<127>>, 32, 64> index_t;
typedef csa_wt<wt_huff<rrr_vector<127>>, 32, 64> meta_index_t;

struct FMIndexShard {
    index_t* data_index;
    size_t* data_offset;
    meta_index_t* meta_index;
    size_t* meta_offset;
    size_t doc_cnt;
};

struct FindResult {
    size_t cnt;
    vector<pair<size_t, size_t>> segment_by_shard; // left-and-right-inclusive
};

struct CountResult {
    size_t count;
};

struct DocResult {
    size_t doc_ix;
    size_t doc_len;
    size_t disp_len;
    size_t needle_offset;
    string meta;
    string data;
};

// struct LocateResult {
//     size_t location;
//     size_t shard_num;
// };

// struct ReconstructResult {
//     string text;
//     size_t shard_num;
//     string metadata;
// };

class Engine {

public:

    Engine (const vector<string> index_dirs, bool load_to_ram, bool get_metadata)
            : _load_to_ram(load_to_ram), _get_metadata(get_metadata) {

        auto start_time = high_resolution_clock::now();

        for (const auto &index_dir : index_dirs) {
            assert (fs::exists(index_dir));

            auto data_index = new index_t();
            auto data_index_path = index_dir + "/data.fm9";
            if (_load_to_ram) {
                load_from_file(*data_index, data_index_path);
            } else {
                load_from_file_(*data_index, data_index_path);
            }
            string data_offset_path = index_dir + "/data_offset";
            int data_fd = open(data_offset_path.c_str(), O_RDONLY);
            assert(data_fd >= 0);
            off_t data_offset_size = lseek(data_fd, 0, SEEK_END);
            assert(data_offset_size > 0);
            size_t* data_offset = (size_t*)mmap(nullptr, data_offset_size, PROT_READ, MAP_PRIVATE, data_fd, 0);
            assert(data_offset != MAP_FAILED);
            size_t doc_cnt = data_offset_size / sizeof(size_t);

            meta_index_t* meta_index;
            size_t* meta_offset;
            if (_get_metadata) {
                meta_index = new meta_index_t();
                auto meta_index_path = index_dir + "/meta.fm9";
                if (_load_to_ram) {
                    load_from_file(*meta_index, meta_index_path);
                } else {
                    load_from_file_(*meta_index, meta_index_path);
                }
                string meta_offset_path = index_dir + "/meta_offset";
                int meta_fd = open(meta_offset_path.c_str(), O_RDONLY);
                assert(meta_fd >= 0);
                off_t meta_offset_size = lseek(meta_fd, 0, SEEK_END);
                assert(meta_offset_size > 0);
                meta_offset = (size_t*)mmap(nullptr, meta_offset_size, PROT_READ, MAP_PRIVATE, meta_fd, 0);
                assert(meta_offset != MAP_FAILED);
            }

            auto shard = FMIndexShard{data_index, data_offset, meta_index, meta_offset, doc_cnt};
            _shards.push_back(shard);
        }

        auto end_time = high_resolution_clock::now();
        auto load_time = duration_cast<milliseconds>(end_time - start_time).count();
        cout << "Load time: " << load_time / 1000.00 << " s." << endl;

        _num_shards = _shards.size();
        assert(_num_shards > 0);
    }

    ~Engine() {

        for (auto& shard : _shards) {
            if (_load_to_ram) {
                delete shard.data_index;
                if (_get_metadata) {
                    delete shard.meta_index;
                }
            } else {
                munmap(shard.data_index, shard.data_index->size());
                if (_get_metadata) {
                    munmap(shard.meta_index, shard.meta_index->size());
                }
            }
            // TODO: munmap *_offset
        }
    }

    FindResult find(const string query) const {

        vector<pair<size_t, size_t>> segment_by_shard(_num_shards);
        if (query.length() == 0) {
            for (size_t s = 0; s < _num_shards; s++) {
                segment_by_shard[s] = {0, _shards[s].data_index->size() - 1};
                // segment_by_shard[s] = {1, _shards[s].data_index->size() - 1}; // start from 1 to exclude the last \0 byte
            }
        } else {
            vector<thread> threads;
            for (size_t s = 0; s < _num_shards; s++) {
                threads.emplace_back(&Engine::_find_thread, this, s, &query, &segment_by_shard[s]);
            }
            for (auto &thread : threads) {
                thread.join();
            }
        }

        size_t cnt = 0;
        for (size_t s = 0; s < _num_shards; s++) {
            assert (segment_by_shard[s].first <= segment_by_shard[s].second + 1);
            cnt += segment_by_shard[s].second + 1 - segment_by_shard[s].first;
        }

        return FindResult{ .cnt = cnt, .segment_by_shard = segment_by_shard, };
    }

    void _find_thread(const size_t s, const string* const query, pair<size_t, size_t>* const segment) const {

        size_t lo = 0;
        size_t hi = 0;
        auto count = sdsl::backward_search(*_shards[s].data_index, 0, _shards[s].data_index->size() - 1, query->begin(), query->end(), lo, hi);
        segment->first = lo;
        segment->second = hi;
    }

    CountResult count(const string& query) const {

        auto find_result = find(query);
        return CountResult{ .count = find_result.cnt, };
    }

    DocResult get_doc_by_rank(const size_t s, const size_t rank, const size_t needle_len, const size_t max_ctx_len) const {

        assert (s < _num_shards);
        const auto &shard = _shards[s];
        assert (rank < shard.data_index->size());

        size_t ptr = (*shard.data_index)[rank];

        size_t lo = 0, hi = shard.doc_cnt;
        while (hi - lo > 1) {
            // _prefetch_doc(shard, lo, hi); // TODO: implement this
            size_t mi = (lo + hi) >> 1;
            size_t p = _convert_doc_ix_to_ptr(shard, mi);
            if (p <= ptr) {
                lo = mi;
            } else {
                hi = mi;
            }
        }
        size_t local_doc_ix = lo;
        size_t doc_ix = 0; for (size_t _ = 0; _ < s; _++) doc_ix += _shards[_].doc_cnt; doc_ix += local_doc_ix;

        size_t doc_start_ptr = _convert_doc_ix_to_ptr(shard, local_doc_ix) + 1; // left-inclusive; +1 because we want to skip the document separator
        size_t doc_end_ptr = _convert_doc_ix_to_ptr(shard, local_doc_ix + 1); // right-exclusive
        size_t doc_len = doc_end_ptr - doc_start_ptr;

        size_t disp_start_ptr = max(doc_start_ptr, ptr < max_ctx_len ? 0 : (ptr - max_ctx_len));
        size_t disp_end_ptr = min(doc_end_ptr, ptr + needle_len + max_ctx_len);
        size_t disp_len = disp_end_ptr - disp_start_ptr;
        size_t needle_offset = ptr - disp_start_ptr;

        string data = "";
        if (disp_start_ptr < disp_end_ptr) {
            data = sdsl::extract(*shard.data_index, disp_start_ptr, disp_end_ptr - 1);
        }

        string meta = "";
        if (_get_metadata) {
            size_t meta_start_ptr = _convert_doc_ix_to_meta_ptr(shard, local_doc_ix); // left-inclusive
            size_t meta_end_ptr = _convert_doc_ix_to_meta_ptr(shard, local_doc_ix + 1) - 1; // right-exclusive; -1 because there is a trailing \n
            if (meta_start_ptr < meta_end_ptr) {
                meta = sdsl::extract(*shard.meta_index, meta_start_ptr, meta_end_ptr - 1);
            }
        }

        return DocResult{ .doc_ix = doc_ix, .doc_len = doc_len, .disp_len = disp_len, .needle_offset = needle_offset, .meta = meta, .data = data, };
    }

    // LocateResult locate(const string& query, size_t num_occ) const {
    //     auto count_result = count(query);
    //     auto counts = count_result.count;
    //     auto count_by_shard = count_result.count_by_shard;
    //     auto lo_by_shard = count_result.lo_by_shard;

    //     if (counts == 0) {
    //         return {SIZE_MAX, SIZE_MAX};
    //     }

    //     if (num_occ > counts) {
    //         throw runtime_error("num_occ is larger than the number of occurrences.");
    //     }

    //     size_t shard_num;
    //     size_t num_occ_prev = 0;
    //     size_t offset;
    //     for (shard_num = 0; shard_num < _num_shards; shard_num++) {
    //         if (num_occ_prev + count_by_shard[shard_num] >= num_occ) break;
    //         num_occ_prev += count_by_shard[shard_num];
    //     }
    //     offset = num_occ - num_occ_prev;

    //     size_t location = (*_shards[shard_num].data_index)[lo_by_shard[shard_num] + offset - 1];
    //     return LocateResult{location, shard_num};
    // }

    // ReconstructResult reconstruct(const string& query, size_t num_occ, size_t pre_text=400, size_t post_text=400) const {
    //     auto locate_result = locate(query, num_occ);
    //     size_t location = locate_result.location;
    //     size_t shard_num = locate_result.shard_num;

    //     if (location == SIZE_MAX) {
    //         return {"", SIZE_MAX};
    //     }

    //     size_t start = (location < pre_text) ? 0 : location - pre_text;
    //     size_t end = std::min(location + query.length() + post_text, _shards[shard_num].data_index->size() - 1);

    //     auto s = sdsl::extract(*_shards[shard_num].data_index, start, end);
    //     // auto s = sdsl::extract(temp_index, location - pre_text, location + query.length() + post_text);
    //     auto pos = s.find("\xff");
    //     while (pos != std::string::npos) {
    //         if (pos > pre_text) {
    //             s.erase(pos);
    //         } else {
    //             s.erase(0, pos+2);
    //             pre_text -= pos + 2;
    //         }
    //         pos = s.find("\xff");
    //     }

    //     if (_get_metadata) {
    //         string metadata = get_metadata(location, shard_num);
    //         return ReconstructResult{s, shard_num, metadata};
    //     } else {
    //         return ReconstructResult{s, shard_num, ""};
    //     }

    // }

    // string get_metadata(size_t location, size_t shard_num) const {
    //     size_t* data_offset = _shards[shard_num].data_offset;
    //     size_t* meta_offset = _shards[shard_num].meta_offset;

    //     // TODO: speed up using binary search
    //     size_t idx = 0;
    //     while (*(data_offset + idx) <= location) {
    //         idx++;
    //     }

    //     if (idx > _shards[shard_num].doc_cnt) {
    //         return "";
    //     }

    //     size_t lower_pos = *(meta_offset + idx - 1);
    //     size_t upper_pos = *(meta_offset + idx);

    //     string meta = sdsl::extract(*_shards[shard_num].metadata, lower_pos, upper_pos);
    //     return meta.substr(0, meta.length()-2);
    // }

private:

    inline size_t _convert_doc_ix_to_ptr(const FMIndexShard& shard, const size_t doc_ix) const {
        assert (doc_ix <= shard.doc_cnt);
        if (doc_ix == shard.doc_cnt) {
            // return shard.data_index->size();
            return shard.data_index->size() - 1; // -1 to exclude the last \0 byte
        }
        return *(shard.data_offset + doc_ix);
    }

    inline size_t _convert_doc_ix_to_meta_ptr(const FMIndexShard& shard, const size_t doc_ix) const {
        assert (doc_ix <= shard.doc_cnt);
        if (doc_ix == shard.doc_cnt) {
            // return shard.meta_index->size();
            return shard.meta_index->size() - 1; // -1 to exclude the last \0 byte
        }
        return *(shard.meta_offset + doc_ix);
    }

private:

    vector<FMIndexShard> _shards;
    size_t _num_shards;
    bool _load_to_ram;
    bool _get_metadata;
};
