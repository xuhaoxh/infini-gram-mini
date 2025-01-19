import os
import sys
import zstandard as zstd

output_dir = f'/weka/oe-training-default/jiachengl/hf-fm/pileval'

os.makedirs(output_dir, exist_ok=True)

raw_path = f'data/pileval.jsonl'
log = os.popen(f'./construct {raw_path} {output_dir}/').read()
print(log)
