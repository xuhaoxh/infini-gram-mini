import multiprocessing as mp
import time
import os
import zstandard as zstd

NUM_PROCESSES = 16
TOTAL_NUM_FILES = 1970
NUM_FILES_PER_SHARD = 1
input_dir = '/weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm'
output_dir = f'/weka/oe-training-default/jiachengl/hf-fm/dclm_f{NUM_FILES_PER_SHARD}_all'

os.makedirs(output_dir, exist_ok=True)

def index_shard(shard_ix):
    raw_path = f'{output_dir}/data_{shard_ix:04d}.jsonl'
    if os.path.exists(raw_path) or os.path.exists(raw_path.replace('.jsonl', '')):
        return
    f_raw = open(raw_path, 'w')
    for file_ix in range(shard_ix * NUM_FILES_PER_SHARD, (shard_ix + 1) * NUM_FILES_PER_SHARD):
        input_path = f'{input_dir}/dclm-{file_ix:04d}.json.zst'
        with open(input_path, 'rb') as f:
            dctx = zstd.ZstdDecompressor()
            with dctx.stream_reader(f) as reader:
                decompressed_data = reader.read().decode('utf-8')
        f_raw.write(decompressed_data.strip('\n') + '\n')
    f_raw.close()
    log = os.popen(f'./construct {raw_path} {output_dir}/').read()
    print(log)
    os.remove(raw_path)
    os.remove(f'{output_dir}/data_{shard_ix:04d}/data_{shard_ix:04d}.text')
    os.remove(f'{output_dir}/data_{shard_ix:04d}/data_{shard_ix:04d}_meta')

with mp.get_context('fork').Pool(NUM_PROCESSES // NUM_FILES_PER_SHARD) as p:
    # p.map(index_shard, list(range(TOTAL_NUM_FILES // NUM_FILES_PER_SHARD)))
    results = []
    for i in range(TOTAL_NUM_FILES // NUM_FILES_PER_SHARD):
        result = p.apply_async(index_shard, args=(i,))
        results.append(result)
        time.sleep(0.1)
    p.close()
    p.join()