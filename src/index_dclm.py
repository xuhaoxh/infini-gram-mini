import os
import sys
import zstandard as zstd

SHARD_IX = int(sys.argv[1])
input_dir = '/weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm'
output_dir = f'/weka/oe-training-default/jiachengl/hf-fm/dclm_f1_all'

os.makedirs(output_dir, exist_ok=True)

raw_path = f'{output_dir}/data_{SHARD_IX:04d}.jsonl'
f_raw = open(raw_path, 'w')
input_path = f'{input_dir}/dclm-{SHARD_IX:04d}.json.zst'
with open(input_path, 'rb') as f:
    dctx = zstd.ZstdDecompressor()
    with dctx.stream_reader(f) as reader:
        decompressed_data = reader.read().decode('utf-8')
f_raw.write(decompressed_data.strip('\n') + '\n')
f_raw.close()
log = os.popen(f'./construct {raw_path} {output_dir}/').read()
print(log)
os.remove(raw_path)
