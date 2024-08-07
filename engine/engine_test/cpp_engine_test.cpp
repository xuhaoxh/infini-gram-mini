// g++ -std=c++17 -O3 engine_test/cpp_engine_test.cpp -o engine_test/cpp_engine_test -I../sdsl/include -L../sdsl/lib -lsdsl -ldivsufsort -ldivsufsort64 -pthread 

#include "../fm_engine/cpp_engine.h"

int main() {
    auto engine = Engine({"/mmfs1/gscratch/h2lab/xuhao/fm_index/indexes/pile-train"});

    {
        cout << "count, empty query" << endl;
        string query = "";
        cout << "query: " << query<< endl;
        auto result = engine.count(query);
        cout << "count: " << result.count << endl;
        cout << "count_by_shard: " << result.count_by_shard << endl;
        cout << "lo_by_shard: " << result.lo_by_shard << endl;
        cout << endl;
    }
    {
        cout << "count, simple query" << endl;
        string query = "natural language processing";
        cout << "query: " << query<< endl;
        auto result = engine.count(query);
        cout << "count: " << result.count << endl;
        cout << "count_by_shard: " << result.count_by_shard << endl;
        cout << "lo_by_shard: " << result.lo_by_shard << endl;
        cout << endl;
    }
    {
        cout << "count, simple query, zero_count" << endl;
        string query = "\xfb";
        cout << "query: " << query<< endl;
        auto result = engine.count(query);
        cout << "count: " << result.count << endl;
        cout << "count_by_shard: " << result.count_by_shard << endl;
        cout << "lo_by_shard: " << result.lo_by_shard << endl;
        cout << endl;
    }

    {
        cout << "reconstruct, simple query" << endl;
        string query = "natural language processing";
        cout << "query: " << query<< endl;
        auto result = engine.reconstruct(query, 10);
        cout << "text: " << result.text << endl;
        cout << "shard_num: " << result.shard_num << endl;
        cout << endl;
    }
}
