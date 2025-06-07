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
namespace fs = filesystem;
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
    vector<pair<size_t, size_t>> segment_by_shard; // left inclusive, right exclusive
};

struct CountResult {
    size_t count;
};

struct DocResult {
    size_t doc_ix;
    size_t doc_len;
    size_t disp_len;
    size_t needle_offset;
    string metadata;
    string text;
};

class Engine {

public:

    Engine (const vector<string> index_dirs, bool load_to_ram, bool get_metadata)
            : _load_to_ram(load_to_ram), _get_metadata(get_metadata) {

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
                segment_by_shard[s] = {0, _shards[s].data_index->size()};
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
            assert (segment_by_shard[s].first <= segment_by_shard[s].second);
            cnt += segment_by_shard[s].second - segment_by_shard[s].first;
        }

        return FindResult{ .cnt = cnt, .segment_by_shard = segment_by_shard, };
    }

    void _find_thread(const size_t s, const string* const query, pair<size_t, size_t>* const segment) const {

        size_t lo = 0;
        size_t hi = 0;
        auto count = sdsl::backward_search(*_shards[s].data_index, 0, _shards[s].data_index->size() - 1, query->begin(), query->end(), lo, hi);
        segment->first = lo;
        segment->second = hi + 1; // so that right end is exclusive
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

        size_t ctx_len = (max_ctx_len - needle_len) / 2;

        size_t disp_start_ptr = max(doc_start_ptr, ptr < ctx_len ? 0 : (ptr - ctx_len));
        size_t disp_end_ptr = min(doc_end_ptr, ptr + needle_len + ctx_len);
        size_t disp_len = disp_end_ptr - disp_start_ptr;
        size_t needle_offset = ptr - disp_start_ptr;

        string text = "";
        if (disp_start_ptr < disp_end_ptr) {
            text = sdsl::extract(*shard.data_index, disp_start_ptr, disp_end_ptr - 1);
            // text = parallel_extract(s, disp_start_ptr, disp_end_ptr, false);
        }

        string metadata = "";
        if (_get_metadata) {
            size_t meta_start_ptr = _convert_doc_ix_to_meta_ptr(shard, local_doc_ix); // left-inclusive
            size_t meta_end_ptr = _convert_doc_ix_to_meta_ptr(shard, local_doc_ix + 1) - 1; // right-exclusive; -1 because there is a trailing \n
            if (meta_start_ptr < meta_end_ptr) {
                metadata = sdsl::extract(*shard.meta_index, meta_start_ptr, meta_end_ptr - 1);
                // metadata = parallel_extract(s, meta_start_ptr, meta_end_ptr, true);
            }
        }

        return DocResult{ .doc_ix = doc_ix, .doc_len = doc_len, .disp_len = disp_len, .needle_offset = needle_offset, .metadata = metadata, .text = text, };
    }

    string parallel_extract(size_t shard_index, size_t disp_start_ptr, size_t disp_end_ptr, bool is_meta) const {
        if (disp_start_ptr >= disp_end_ptr) return "";

        const size_t total_len = disp_end_ptr - disp_start_ptr;

        if (total_len < 100) {
            if (is_meta) {
                return sdsl::extract(*_shards[shard_index].meta_index, disp_start_ptr, disp_end_ptr - 1); // inclusive
            } else {
                return sdsl::extract(*_shards[shard_index].data_index, disp_start_ptr, disp_end_ptr - 1); // inclusive
            }
        }

        const size_t num_threads = min(total_len / 100, size_t(10));

        const size_t chunk_size = (total_len + num_threads - 1) / num_threads;

        vector<string> segments(num_threads);
        vector<thread> threads;

        for (size_t i = 0; i < num_threads; ++i) {
            const size_t start = disp_start_ptr + i * chunk_size;
            if (start >= disp_end_ptr) {
                segments[i] = "";
                continue;
            }

            const size_t end = min(start + chunk_size, disp_end_ptr);
            threads.emplace_back(&Engine::_extract_thread, this, shard_index, start, end, &segments[i], is_meta);
        }

        for (auto &t : threads) {
            t.join();
        }
        
        string result;
        for (auto &seg : segments) {
            result += seg;
        }

        return result;
    }

    void _extract_thread(size_t shard_index, size_t start, size_t end, string* out, bool is_meta) const {
        if (is_meta) {
            *out = sdsl::extract(*_shards[shard_index].meta_index, start, end - 1); // inclusive
        } else {
            *out = sdsl::extract(*_shards[shard_index].data_index, start, end - 1); // inclusive
        }
    }

private:

    inline size_t _convert_doc_ix_to_ptr(const FMIndexShard& shard, const size_t doc_ix) const {
        assert (doc_ix <= shard.doc_cnt);
        if (doc_ix == shard.doc_cnt) {
            return shard.data_index->size() - 1; // -1 to exclude the last \0 byte
        }
        return *(shard.data_offset + doc_ix);
    }

    inline size_t _convert_doc_ix_to_meta_ptr(const FMIndexShard& shard, const size_t doc_ix) const {
        assert (doc_ix <= shard.doc_cnt);
        if (doc_ix == shard.doc_cnt) {
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
