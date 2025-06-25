import numpy as np
import random
import time
import json
import sys

sys.path.append('../engine')
from src.engine import InfiniGramMiniEngine

def main():
    engine = InfiniGramMiniEngine(index_dirs=["../index/v2_pileval"], load_to_ram=False, get_metadata=True)

    query = "nature"
    print(engine.count(query))
    print()
    print(engine.find(query))
    print()
    print(engine.get_doc_by_rank(s=0, rank=909665931, needle_len=len(query), max_ctx_len=20))

if __name__ == '__main__':
    main()
