API Endpoint
============

The infini-gram mini API endpoint is ``https://api-mini.infini-gram.io/``.
Please make regular HTTP POST requests.
In your request, please include a JSON payload, and the response will also contain a JSON payload.

Most queries are processed within seconds.
You should receive the response within a couple seconds.
If you are experiencing longer latencies, it might be due to network delays or heavy traffic.

**Please wrap your requests in an exception-handling loop. We can't guarantee 100% uptime or successful processing of queries, so it may be wise to catch errors and retry the failed requests.**

Getting Started
---------------

**From Shell:**

.. code-block:: bash

    curl -X POST -H "Content-Type: application/json" \
        -d '{"index": "v2_cc-2025-05", "query_type": "count", "query": "University of Washington"}' \
        https://api-mini.infini-gram.io

Outputs:

.. code-block:: json

    {"count":893632,"latency":522.6130485534668}

**From Python:**

.. code-block:: python

    import requests

    payload = {
        'index': 'v2_dclm_all',
        'query_type': 'count',
        'query': 'University of Washington',
    }
    result = requests.post('https://api-mini.infini-gram.io/', json=payload).json()
    print(result)

Outputs::

    {'count': 3202538, 'latency': 721.5242385864258}

Overview
--------

**Available indexes:**

We have built the infini-gram mini indexes on several corpora, and you may query them through the API.

.. list-table::
   :header-rows: 1

   * - Name
     - Documents
     - Size
     - Corpus
    * - ``v2_cc_2025-08``
     - 2,679,706,056
     - 8,163,085,167,146
     - `Common Crawl February 2025 Crawl <https://data.commoncrawl.org/crawl-data/CC-MAIN-2025-08/index.html>`_
   * - ``v2_cc_2025-05``
     - 3,031,278,337
     - 9,078,722,426,438
     - `Common Crawl January 2025 Crawl <https://data.commoncrawl.org/crawl-data/CC-MAIN-2025-05/index.html>`_
   * - ``v2_dclm_all``
     - 2,948,096,911
     - 16,666,308,904,212
     - `DCLM-baseline <https://huggingface.co/datasets/mlfoundations/dclm-baseline-1.0>`_
   * - ``v2_piletrain``
     - 210,607,728
     - 1,307,910,802,542
     - `DCLM-baseline <https://huggingface.co/datasets/EleutherAI/pile>`_
   * - ``v2_pileval``
     - 214,670
     - 1,344,908,273
     - `DCLM-baseline <https://huggingface.co/datasets/EleutherAI/pile>`_
        

**Input parameters:**

In general, the request JSON payload should be a dict containing the following fields:

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Acceptable Values
   * - ``index``
     - The index to search in
     - E.g., ``v2_dclm_all``. See full list in the table in the "Available indexes" section above.
   * - ``query_type``
     - One of the supported query types
     - ``count``, ``find``, ``get_doc_by_rank``
   * - ``query``
     - The query string
     - ``query``: Any string. (Empty may be OK depending on query type)

For certain query types, additional fields may be required.
Please see the specific query type below for more details.

The query string is processed at the character level, so it is acceptable to end a query at any arbitrary character boundary. Note that queries are case-sensitive.

**Output parameters:**

If an error occurred (e.g., malformed input, internal server error), the response JSON dict will contain a key ``error`` with a string value describing the error.
Please check this key first before processing the rest of the response.

If the query was successful, the response JSON dict will contain the following fields:

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Value Range
   * - ``latency``
     - The processing time in the engine. This does not include network latency.
     - A non-negative float number, in milliseconds

In addition, the response JSON dict will contain results specific to the query type.
Please see the specific query type below for more details.

Query Types
-----------

1. Count a string
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This query type counts the number of times the query string appears in the corpus.
If the query is an empty string, the total number of tokens in the corpus will be returned.

You can simply enter a string, in which we count the number of occurrences of the string.

**Examples:**

If you query ``natural language processing``, the API returns the number of occurrences of ``natural language processing``.

**Input parameters:**

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Acceptable Values
   * - ``index``
     - see overview
     - see overview
   * - ``query_type``
     - see overview
     - ``count``
   * - ``query``
     - The query string
     - A string (empty is OK).


**Output parameters:**

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Value Range
   * - ``count``
     - The number of occurrences of the query
     - A non-negative integer
   * - ``latency``
     - see overview
     - see overview
   

2. Search documents
~~~~~~~~~~~~~~~~~~~

This query type returns documents in the corpus that match your query. The engine can return documents containing a single query.


**Examples:**

1. If you query ``natural language processing``, the documents returned would contain the string ``natural language processing``.

**Step 1: find**

First, you need to make a ``find`` query to get information about where the matching documents are located.

**Input parameters:**

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Acceptable Values
   * - ``index``
     - see overview
     - see overview
   * - ``query_type``
     - see overview
     - ``find``
   * - ``query``
     - The search query
     - A non-empty string

**Output parameters:**

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Value Range
   * - ``latency``
     - see overview
     - see overview
   * - ``cnt``
     - The number of occurrences of the query
     - A non-negative integer
   * - ``segment_by_shard``
     - The segment of each suffix array shard that matches the query
     - A list of 2-tuples, each tuple is a pair of non-negative integers, where the second integer is no smaller than the first integer

The returned ``segment_by_shard`` is a list of 2-tuples, each tuple represents a range of "ranks" in one of the shards of the index, and each rank can be traced back to a matched document in that shard.
The length of this list is equal to the total number of shards.

**Step 2: get_doc_by_rank**

Then, you can use the ``get_doc_by_rank`` query to retrieve a matching document by any rank in the segment.

**Input parameters:**

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Acceptable Values
   * - ``index``
     - see overview
     - see overview
   * - ``query_type``
     - see overview
     - ``get_doc_by_rank``
   * - ``s``
     - The shard index
     - An integer in range [0, ``len(segment_by_shard)``)
   * - ``rank``
     - A rank in the shard
     - An integer in range [``segment_by_shard[s][0]``, ``segment_by_shard[s][1]``)
   * - ``max_ctx_len``
     - The maximum number of characters preceding and following the query term to return. Total returned length will be ``2 * max_ctx_len + query length``.
     - An integer in range [1, 10000], default = 1000

For example, if you want to retrieve the first matched document in shard 0, you should make the query with ``s=0`` and ``rank=segment_by_shard[0][0]``.

**Output parameters:**

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Value Range
   * - ``latency``
     - see overview
     - see overview
   * - ``doc_ix``
     - The index of the document in the corpus
     - A non-negative integer
   * - ``doc_len``
     - The total number of characters in the document
     - A non-negative integer
   * - ``disp_len``
     - The number of characters returned
     - A non-negative integer
   * - ``needle_offset``
     - The length of context
     - A non-negative integer
   * - ``text``
     - The retrieved document
     - A string of has ``2 * max_ctx_len + query length`` characters
