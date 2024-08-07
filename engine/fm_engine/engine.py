import sys
from typing import Iterable, List, Optional, cast

from fm_engine.models import FmEngineResponse, CountResponse, LocateResponse, ReconstructResponse
from .cpp_engine import Engine

class FmIndexEngine:
    
    def __init__(self, index_dir: Iterable[str] | str,
                 pre_text=400, post_text=400) -> None:
        
        assert sys.byteorder == 'little', 'This code is designed to run on little-endian machines only!'

        if type(index_dir) == str:
            index_dir = [index_dir]
        assert type(index_dir) == list and all(type(d) == str for d in index_dir)

        self.engine = Engine(index_dir, pre_text, post_text)
    
    def count(self, query) -> FmEngineResponse[CountResponse]:
        result = self.engine.count(query)
        return {'count': result.count, 'count_by_shard': result.count_by_shard, 'lo_by_shard': result.lo_by_shard}
    
    def locate(self, query, num_occ) -> FmEngineResponse[LocateResponse]:
        result = self.engine.locate(query, num_occ)
        return {'location': result.location, 'shard_num': result.shard_num}
    
    def reconstruct(self, query, num_occ) -> FmEngineResponse[ReconstructResponse]:
        result = self.engine.reconstruct(query, num_occ)
        return {'text': result.text, 'shard_num': result.shard_num}
    
    # def locate(self, query, num_occ, count_by_shards):
    #     result = self.engine.locate(query, num_occ, count_by_shards)
    #     return {'location': result.location, 'shard_num': result.shard_num}
    
    # def reconstruct(self, location, shard_num, query_len):
    #     result = self.engine.reconstruct(location, shard_num, query_len)
    #     return {'text': result.text, 'location': result.shard_num}

