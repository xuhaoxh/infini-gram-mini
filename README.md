# 📖 Infini-gram mini

This repo hosts the source code of the infini-gram mini search engine, which is described in this paper: Infini-gram mini: Exact n-gram Search at the Internet Scale with FM-Index.

To learn more about infini-gram mini:
* Paper:
* Project Home: 
* Web Interface: 
* API Endpoint: 
* Python Package: 
* Code: 
* Benchmark Contamination Monitoring System:  


## Overview

Infini-gram mini is an engine that processes queries on the largest body of text in the current open-source community (as of May 2025). 
It can count the occurrence of arbitrarily long strings in 45.6 TB of text corpora and retrieve their containing documents in seconds. 

Infini-gram mini is powered by indexes based on FM-Index.
This repo contains everything you might need for constructing an infini-gram mini index of a text corpus, and perform queries on this index.


## Getting Started

To query a local index (e.g., `pile-train`), you need to initialize an engine with the corresponding index and invoke its methods. Below is a step-by-step example.

### 1. Initialize the engine

Create an engine instance using the appropriate index directory. You can configure:

- Whether the index stays on disk (`load_to_ram=False`, uses less RAM but is slower), or is fully loaded into memory (`load_to_ram=True`, uses more RAM but is faster).
- Whether to return metadata for each result (`get_metadata=True`).

```python
from engine.src import InfiniGramMiniEngine

engine = InfiniGramMiniEngine(index_dir="index/v2_piletrain", load_to_ram=False, get_metadata=True)
```

### 2. Counting a query

To count the occurrences of a string `natural language processing` in Pile-train corpus:

```python
query = "natural language processing"
engine.count(query)
#83,470
```

### 3. Retrieving a matching document

First, call `find()` to get information about where the query locates.

```python
engine.find(query)
# {"cnt":83470, "segment_by_shard":[[442381579355,442381620985],[443017902435,443017944275]]}
```
- `segment_by_shard` is a list of `[start, end]` byte ranges in each shard where the query appears.

Then, to retrieve text snippet around the first occurrance in shard 0:
```python
engine.get_doc_by_rank(s=0, rank=442381579355, max_ctx_len=20)
# {"disp_len":67, "doc_ix":48649509, "doc_len":813513, "metadata":{"path": "06.jsonl", "linenum": 6526203, "metadata": {"meta": {"pile_set_name": "HackerNews"}}}, "needle_offset":20, "text":"Research Engineer \\- natural language processing\n\n    \n    \n      - "}
```


## Customizing the engine
If you modify the C++ backend of the engine, follow the steps below to recompile and use your custom version:

### 1. Prerequisites

Make sure you have the following installed:

- A C++ compiler with support for `-std=c++17` 
- The `pybind11` Python package:
  ```bash
  pip install pybind11
  ```

### 2. Compilation
Under `engine` folder, compile with the following command:
```command
c++ -std=c++17 -O3 -shared -fPIC $(python3 -m pybind11 --includes) src/cpp_engine.cpp -o src/cpp_engine$(python3-config --extension-suffix) -I../sdsl/include -L../sdsl/lib -lsdsl -ldivsufsort -ldivsufsort64 -pthread
```

### 3. Import the engine
Once compiled, you can import and use the customized engine in Python:
```python
from engine.src import InfiniGramMiniEngine
```

## Citation
If you find infini-gram mini useful, please kindly cite our paper:

