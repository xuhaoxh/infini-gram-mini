from typing import Any, Iterable, List, Sequence, Tuple, TypeAlias, TypeVar, TypedDict

class ErrorResponse(TypedDict):
    error: str 

T = TypeVar("T")
FmEngineResponse: TypeAlias = ErrorResponse | T 

class CountResponse(TypedDict):
    count: int
    count_by_shard: List[int]
    lo_by_shard: List[int]

class LocateResponse(TypedDict):
    location: int
    shard_num: int

class ReconstructResponse(TypedDict):
    text: str
    shard_num: int
    metadata: str

