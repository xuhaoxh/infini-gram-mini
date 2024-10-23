// g++ -std=c++17 -O3 engine_test/cpp_engine_test.cpp -o engine_test/cpp_engine_test -I../sdsl/include -L../sdsl/lib -lsdsl -ldivsufsort -ldivsufsort64 -pthread 

#include "../fm_engine/cpp_engine.h"
#include <iostream>
#include <chrono>

int main() {
    auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/indexes/pile-train/"}, false);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/experiments/pile_val"}, false);
    // auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/experiments/test/"}, true);

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

    {
        cout << "count, simple query" << endl;
        // string query = "natural language processing";
        string query = "3.14";
        cout << "query: " << query<< endl;
        auto start = std::chrono::high_resolution_clock::now();
        auto result = engine.count(query);
        auto end = std::chrono::high_resolution_clock::now();
        cout << "count: " << result.count << endl;
        cout << "count_by_shard: " << result.count_by_shard << endl;
        cout << "lo_by_shard: " << result.lo_by_shard << endl;

        auto time = duration_cast<milliseconds>(end - start).count();
        cout << "time taken: " << time << " ms." << endl;
        
        cout << endl;
    }
    // {
    //     cout << "locate, simple query" << endl;
    //     // string query = "natural language processing";
    //     string query = "3.14";
    //     cout << "query: " << query<< endl;
    //     auto start = std::chrono::high_resolution_clock::now();
    //     auto result = engine.locate(query, 752);
    //     auto end = std::chrono::high_resolution_clock::now();
    //     cout << "location: " << result.location << endl;
    //     cout << "shard_num: " << result.shard_num << endl;

    //     auto time = duration_cast<milliseconds>(end - start).count();
    //     cout << "time taken: " << time << " ms." << endl;

    //     cout << endl;
    // }
    {
        cout << "reconstruct, simple query" << endl;
        // string query = "natural language processing";
        string query = "3.14";
        // string query = "Our study strongly suggests";
        cout << "query: " << query<< endl;
        auto start = std::chrono::high_resolution_clock::now();
        auto result = engine.reconstruct(query, 1, 100, 100);
        auto end = std::chrono::high_resolution_clock::now();
        cout << "text: " << result.text << endl;
        cout << "shard_num: " << result.shard_num << endl;

        auto time = duration_cast<milliseconds>(end - start).count();
        cout << "time taken: " << time << " ms." << endl;

        cout << endl;
    }
    {
        cout << "reconstruct, simple query, same shard" << endl;
        // string query = "natural language processing";
        string query = "3.14";
        // string query = "Our study strongly suggests";
        cout << "query: " << query<< endl;
        auto start = std::chrono::high_resolution_clock::now();
        auto result = engine.reconstruct(query, 10, 100, 100);
        auto end = std::chrono::high_resolution_clock::now();
        cout << "text: " << result.text << endl;
        cout << "shard_num: " << result.shard_num << endl;

        auto time = duration_cast<milliseconds>(end - start).count();
        cout << "time taken: " << time << " ms." << endl;

        cout << endl;
    }
    {
        cout << "reconstruct, simple query, different shard" << endl;
        // string query = "natural language processing";
        string query = "3.14";
        // string query = "Our study strongly suggests";
        cout << "query: " << query<< endl;
        auto start = std::chrono::high_resolution_clock::now();
        auto result = engine.reconstruct(query, 60000, 100, 100);
        auto end = std::chrono::high_resolution_clock::now();
        cout << "text: " << result.text << endl;
        cout << "shard_num: " << result.shard_num << endl;

        auto time = duration_cast<milliseconds>(end - start).count();
        cout << "time taken: " << time << " ms." << endl;

        cout << endl;
    }
}
