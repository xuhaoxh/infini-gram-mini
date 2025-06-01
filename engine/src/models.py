from typing import List, Tuple, TypeAlias, TypeVar, TypedDict

class ErrorResponse(TypedDict):
    error: str

T = TypeVar("T")
EngineResponse: TypeAlias = ErrorResponse | T

class FindResponse(TypedDict):
    cnt: int
    segment_by_shard: List[Tuple[int, int]]

class CountResponse(TypedDict):
    count: int

class DocResponse(TypedDict):
    doc_ix: int
    doc_len: int
    disp_len: int
    needle_offset: int
    metadata: str
    text: str

