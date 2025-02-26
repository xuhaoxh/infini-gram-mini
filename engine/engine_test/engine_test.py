# python engine_test/engine_test.py
# python -m engine_test.engine_test

import numpy as np
import random
import time
import json

from fm_engine.engine import FmIndexEngine

def main():
    # engine = FmIndexEngine(index_dir="/mmfs1/gscratch/h2lab/xuhao/fm-engine/indexes/pile-train", load_to_ram=False, get_metadata=True)
    engine = FmIndexEngine(index_dir="/mmfs1/gscratch/h2lab/xuhao/fm-engine/indexes/val", load_to_ram=True, get_metadata=True)
    # engine = FmIndexEngine(index_dir="/mmfs1/gscratch/h2lab/xuhao/fm-engine/indexes/test", load_to_ram=False, get_metadata=True)

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

    query = "nature"
    print(engine.count(query))
    print()
    # print(engine.locate(query, 100))
    # print()
    idx = random.randint(1, 1000)
    print(f"idx: {idx}")
    print(engine.reconstruct(query, 1, 100, 100))
    print()

if __name__ == '__main__':
    main()
