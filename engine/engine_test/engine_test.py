# python engine_test/engine_test.py
# python -m engine_test.engine_test

import numpy as np
import random
import time
import json

from engine.src import InfiniGramMiniEngine

def main():
    engine = InfiniGramMiniEngine(index_dir="../index/v2_pileval", load_to_ram=True, get_metadata=True)

    query = "nature"
    print(engine.count(query))
    print()
    idx = random.randint(1, 1000)
    print(f"idx: {idx}")
    print(engine.reconstruct(query, 1, 100, 100))
    print()

if __name__ == '__main__':
    main()
