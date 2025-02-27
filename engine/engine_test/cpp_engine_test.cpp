// g++ -std=c++17 -O3 engine_test/cpp_engine_test.cpp -o engine_test/cpp_engine_test -I../sdsl/include -L../sdsl/lib -lsdsl -ldivsufsort -ldivsufsort64 -pthread

#include "../fm_engine/cpp_engine.h"
#include <iostream>
#include <chrono>
#include <random>

int main() {
    auto engine = Engine({"../index/pileval"}, false, true);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm-engine/indexes/pile-val"}, false, true);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm-engine/indexes/dclm-0000"}, false, true);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm-engine/indexes/test"}, false, true);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm-engine/indexes/pile-train"}, false, true);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/indexes/pile-val-512"}, false);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/indexes/pile-val"}, false);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/indexes/pile-train-0"}, false);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/experiments/test/"}, false);

    {
        string query = "natural language processing";
        // string query = "";
        cout << "query: " << query << endl;
        auto find_result = engine.find(query);
        cout << "count: " << find_result.cnt << endl;

        auto [start, end] = find_result.segment_by_shard[0];
        auto doc = engine.get_doc_by_rank(0, start, query.length(), 20);
        cout << "doc_ix: " << doc.doc_ix << endl;
        cout << "doc_len: " << doc.doc_len << endl;
        cout << "disp_len: " << doc.disp_len << endl;
        cout << "needle_offset: " << doc.needle_offset << endl;
        cout << "meta: " << doc.meta << endl;
        cout << "data: " << doc.data << endl;

        doc = engine.get_doc_by_rank(0, end, query.length(), 20);
        cout << "doc_ix: " << doc.doc_ix << endl;
        cout << "doc_len: " << doc.doc_len << endl;
        cout << "disp_len: " << doc.disp_len << endl;
        cout << "needle_offset: " << doc.needle_offset << endl;
        cout << "meta: " << doc.meta << endl;
        cout << "data: " << doc.data << endl;
    }
    // {
    //     string query = "natural language processing";
    //     cout << "query: " << query << endl;
    //     auto count_result = engine.count(query);
    //     cout << "count: " << count_result.count << endl;
    //     random_device rd;
    //     mt19937 gen(rd());
    //     uniform_int_distribution<> dist(1, count_result.count);
    //     int random_idx = dist(gen);

    //     cout << "idx: " << random_idx << endl;
    //     auto start = std::chrono::high_resolution_clock::now();
    //     auto result = engine.reconstruct(query, random_idx, 100, 100);
    //     auto end = std::chrono::high_resolution_clock::now();
    //     cout << "text: " << result.text << endl;
    //     cout << "shard_num: " << result.shard_num << endl;
    //     cout << "meta_data: " << result.metadata << endl;

    //     auto time = duration_cast<milliseconds>(end - start).count();
    //     cout << "time taken: " << time << " ms." << endl;

    //     cout << endl;
    // }
    // {
    //     cout << "count, empty query" << endl;
    //     string query = "";
    //     cout << "query: " << query<< endl;
    //     auto result = engine.count(query);
    //     cout << "count: " << result.count << endl;
    //     cout << "count_by_shard: " << result.count_by_shard << endl;
    //     cout << "lo_by_shard: " << result.lo_by_shard << endl;
    //     cout << endl;
    // }
    // {
    //     cout << "count, simple query, zero_count" << endl;
    //     string query = "\xfb";
    //     cout << "query: " << query<< endl;
    //     auto result = engine.count(query);
    //     cout << "count: " << result.count << endl;
    //     cout << "count_by_shard: " << result.count_by_shard << endl;
    //     cout << "lo_by_shard: " << result.lo_by_shard << endl;
    //     cout << endl;
    // }
    // {
    //     cout << "reconstuct, simple query, zero_count" << endl;
    //     string query = "\xfb";
    //     cout << "query: " << query<< endl;
    //     auto result = engine.reconstruct(query, 1, 200, 200);
    //     cout << "text: " << result.text << endl;
    //     cout << "shard_num: " << result.shard_num << endl;
    //     cout << endl;
    // }

    // {
    //     cout << "count, simple query" << endl;
    //     // string query = "natural language processing";
    //     // string query = "3.14";
    //     string query = "\xff";
    //     cout << "query: " << query<< endl;
    //     auto start = std::chrono::high_resolution_clock::now();
    //     auto result = engine.count(query);
    //     auto end = std::chrono::high_resolution_clock::now();
    //     cout << "count: " << result.count << endl;
    //     cout << "count_by_shard: " << result.count_by_shard << endl;
    //     cout << "lo_by_shard: " << result.lo_by_shard << endl;

    //     auto time = duration_cast<milliseconds>(end - start).count();
    //     cout << "time taken: " << time << " ms." << endl;

    //     cout << endl;
    // }
    // {
    //     cout << "locate, simple query" << endl;
    //     // string query = "natural language processing";
    //     string query = "3.14";
    //     cout << "query: " << query<< endl;
    //     auto start = std::chrono::high_resolution_clock::now();
    //     auto result = engine.locate(query, 1);
    //     auto end = std::chrono::high_resolution_clock::now();
    //     cout << "location: " << result.location << endl;
    //     cout << "shard_num: " << result.shard_num << endl;

    //     auto time = duration_cast<milliseconds>(end - start).count();
    //     cout << "time taken: " << time << " ms." << endl;

    //     cout << endl;
    // }
    // {
    //     cout << "reconstruct, simple query, same shard" << endl;
    //     // string query = "natural language processing";
    //     string query = "3.14";
    //     // string query = "Our study strongly suggests";
    //     cout << "query: " << query<< endl;
    //     auto start = std::chrono::high_resolution_clock::now();
    //     auto result = engine.reconstruct(query, 10, 100, 100);
    //     auto end = std::chrono::high_resolution_clock::now();
    //     cout << "text: " << result.text << endl;
    //     cout << "shard_num: " << result.shard_num << endl;

    //     auto time = duration_cast<milliseconds>(end - start).count();
    //     cout << "time taken: " << time << " ms." << endl;

    //     cout << endl;
    // }
    // {
    //     cout << "reconstruct, simple query, different shard" << endl;
    //     // string query = "natural language processing";
    //     string query = "3.14";
    //     // string query = "Our study strongly suggests";
    //     cout << "query: " << query<< endl;
    //     auto start = std::chrono::high_resolution_clock::now();
    //     auto result = engine.reconstruct(query, 60000, 100, 100);
    //     auto end = std::chrono::high_resolution_clock::now();
    //     cout << "text: " << result.text << endl;
    //     cout << "shard_num: " << result.shard_num << endl;

    //     auto time = duration_cast<milliseconds>(end - start).count();
    //     cout << "time taken: " << time << " ms." << endl;

    //     cout << endl;
    // }
}
