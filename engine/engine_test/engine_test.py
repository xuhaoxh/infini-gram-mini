# python engine_test/engine_test.py
# python -m engine_test.engine_test

import numpy as np
import random
import time
import json

from fm_engine.engine import FmIndexEngine

def main():
    engine = FmIndexEngine(index_dir="/mmfs1/gscratch/h2lab/xuhao/fm_index/experiments/pile_val")

    # with open("../src/query.json", 'r', encoding='utf-8') as file:
    #     queries = json.load(file)
    
    # times = []
    # counts = []
    # for i, query in enumerate(queries):
    #     start_time = time.time()
    #     count_result = engine.count(query)
    #     end_time = time.time()
    #     times.append(end_time - start_time)
    #     counts.append(count_result['count'])
    # print('Average time:', np.mean(times))
    # print('Total count: ', np.sum(counts))
    # exit()

    query = "natural language processing"
    print(engine.count(query))
    print()
    print(engine.locate(query, 100))
    print()
    print(engine.reconstruct(query, 30000, 200, 400))
    print()

if __name__ == '__main__':
    main()
