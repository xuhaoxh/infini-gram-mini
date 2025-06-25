// g++ -std=c++17 -O3 engine_test/cpp_engine_test.cpp -o engine_test/cpp_engine_test -I../sdsl/include -L../sdsl/lib -lsdsl -ldivsufsort -ldivsufsort64 -pthread

#include "../src/cpp_engine.h"
#include <iostream>
#include <chrono>
#include <random>

int main() {
    auto engine = Engine({"../index/v2_pileval"}, false, true);

    {
        string query = "natural language processing";
        cout << "query: " << query << endl;
        auto find_result = engine.find(query);
        cout << "count: " << find_result.cnt << endl;

        auto [start, end] = find_result.segment_by_shard[0];
        auto doc = engine.get_doc_by_rank(0, start, query.length(), 20);
        cout << "doc_ix: " << doc.doc_ix << endl;
        cout << "doc_len: " << doc.doc_len << endl;
        cout << "disp_len: " << doc.disp_len << endl;
        cout << "needle_offset: " << doc.needle_offset << endl;
        cout << "text: " << doc.text << endl;

        auto start_time = std::chrono::high_resolution_clock::now();
        doc = engine.get_doc_by_rank(0, end-1, query.length(), 50);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto time = duration_cast<milliseconds>(end_time - start_time).count();
        cout << "time taken: " << time << " ms." << endl;
        cout << "doc_ix: " << doc.doc_ix << endl;
        cout << "doc_len: " << doc.doc_len << endl;
        cout << "disp_len: " << doc.disp_len << endl;
        cout << "needle_offset: " << doc.needle_offset << endl;
        cout << "text: " << doc.text << endl;
    }
}
