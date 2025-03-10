import os
import subprocess

data_dir = 's3://ai2-llm/pretraining-data/sources/dclm/pool/documents/CC-MAIN-2025-05/segments'
output = os.popen(f'aws s3 ls {data_dir}/').read()
lines = output.split('\n')
lines = [line for line in lines if 'PRE ' in line]
subdirs = [line.split('PRE ')[-1].rstrip('/') for line in lines]
subdirs = sorted(subdirs, key=lambda x: int(x.split('.')[-1]))
for subdir in subdirs:
    output = os.popen(f'aws s3 ls {data_dir}/{subdir}/ --summarize | grep "Total Size"').read()
    size = output.split('Total Size: ')[-1].strip()
    print(f'{subdir}: {int(size) // 1e9} GB')
