import sys
from typing import Iterable, List, Optional, cast

from src.models import EngineResponse, FindResponse, CountResponse, DocResponse
from .cpp_engine import Engine

class InfiniGramMiniEngine:

    def __init__(self, index_dirs: Iterable[str], load_to_ram: bool, get_metadata: bool) -> None:

        assert sys.byteorder == 'little', 'This code is designed to run on little-endian machines only!'
        assert type(index_dirs) == list and all(type(d) == str for d in index_dirs)

        self.engine = Engine(index_dirs, load_to_ram, get_metadata)

    def find(self, query: str) -> EngineResponse[FindResponse]:
        result = self.engine.find(query)
        return {'cnt': result.cnt, 'segment_by_shard': result.segment_by_shard}

    def count(self, query: str) -> EngineResponse[CountResponse]:
        result = self.engine.count(query)
        return {'count': result.count}

    def get_doc_by_rank(self, s: int, rank: int, needle_len: int, max_ctx_len: int) -> EngineResponse[DocResponse]:
        result = self.engine.get_doc_by_rank(s, rank, needle_len, max_ctx_len)
        try:
            text = result.text
        except:
            return {'error': 'Failed to decode document text with UTF-8. This is likely because the context was cut off in the middle of a multi-byte char. Please try with a different max context length.'}
        return {
            'doc_ix': result.doc_ix,
            'doc_len': result.doc_len,
            'disp_len': result.disp_len,
            'needle_offset': result.needle_offset,
            'metadata': result.metadata,
            'text': result.text,
        }
