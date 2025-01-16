import os
import sys
import zstandard as zstd

input_dir = '/weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm'
raw_dir = '/weka/oe-training-default/jiachengl/hf-fm/dclm/raw'
index_dir = '/weka/oe-training-default/jiachengl/hf-fm/dclm/index/'
NUM_FILES_PER_SHARD = int(sys.argv[1])
TOTAL_FILES = 1970
NUM_SHARDS = int(sys.argv[2])

for s in range(NUM_SHARDS):
    raw_path = f'{raw_dir}/data_{s:02d}.jsonl'
    f_raw = open(raw_path, 'w')
    for i in range(s * NUM_FILES_PER_SHARD, min((s + 1) * NUM_FILES_PER_SHARD, TOTAL_FILES)):
        input_path = f'{input_dir}/dclm-{i:04d}.json.zst'
        with open(input_path, 'rb') as f:
            dctx = zstd.ZstdDecompressor()
            with dctx.stream_reader(f) as reader:
                decompressed_data = reader.read().decode('utf-8')
        f_raw.write(decompressed_data.strip('\n') + '\n')
    f_raw.close()
    log = os.popen(f'./construct {raw_path} {index_dir}').read()
    print(log)
