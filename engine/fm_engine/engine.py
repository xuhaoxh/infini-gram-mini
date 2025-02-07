import sys
from typing import Iterable, List, Optional, cast

from fm_engine.models import FmEngineResponse, CountResponse, LocateResponse, ReconstructResponse
from .cpp_engine import Engine

class FmIndexEngine:
    
    def __init__(self, index_dir: Iterable[str] | str, load_to_ram: bool, get_metadata: bool) -> None:
        
        assert sys.byteorder == 'little', 'This code is designed to run on little-endian machines only!'

        if type(index_dir) == str:
            index_dir = [index_dir]
        assert type(index_dir) == list and all(type(d) == str for d in index_dir)

        self.engine = Engine(index_dir, load_to_ram, get_metadata)
    
    def count(self, query) -> FmEngineResponse[CountResponse]:
        result = self.engine.count(query)
        return {'count': result.count, 'count_by_shard': result.count_by_shard, 'lo_by_shard': result.lo_by_shard}
    
    def locate(self, query, num_occ) -> FmEngineResponse[LocateResponse]:
        result = self.engine.locate(query, num_occ)
        return {'location': result.location, 'shard_num': result.shard_num}
    
    def reconstruct(self, query, num_occ, pre_text, post_text) -> FmEngineResponse[ReconstructResponse]:
        result = self.engine.reconstruct(query, num_occ, pre_text, post_text)
        return {'text': result.text, 'shard_num': result.shard_num, 'metadata': result.metadata}
        