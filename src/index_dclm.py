import os
import sys
import zstandard as zstd

TOTAL_FILES = 1970
NUM_FILES_PER_SHARD = int(sys.argv[1])
NUM_SHARDS = int(sys.argv[2])
input_dir = '/weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm'
output_dir = f'/weka/oe-training-default/jiachengl/hf-fm/dclm_f{NUM_FILES_PER_SHARD}_s{NUM_SHARDS}'

os.makedirs(output_dir, exist_ok=True)

for s in range(NUM_SHARDS):
    raw_path = f'{output_dir}/data_{s:02d}.jsonl'
    f_raw = open(raw_path, 'w')
    for i in range(s * NUM_FILES_PER_SHARD, min((s + 1) * NUM_FILES_PER_SHARD, TOTAL_FILES)):
        input_path = f'{input_dir}/dclm-{i:04d}.json.zst'
        with open(input_path, 'rb') as f:
            dctx = zstd.ZstdDecompressor()
            with dctx.stream_reader(f) as reader:
                decompressed_data = reader.read().decode('utf-8')
        f_raw.write(decompressed_data.strip('\n') + '\n')
    f_raw.close()
    log = os.popen(f'./construct {raw_path} {output_dir}/').read()
    print(log)
    os.remove(raw_path)
