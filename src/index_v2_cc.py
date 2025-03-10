import argparse
import multiprocessing as mp
import os
import resource
import subprocess
import sys
from tqdm import tqdm
from indexing import prepare, build_sa_bwt

def main():

    for s in range(15):
        save_dir = f'/weka/oe-training-default/jiachengl/hf-fm/index/v2_cc/2025-05/{s:02d}'
        if os.path.exists(save_dir):
            print(f'Index shard {s:02d} already exists. Skipping.', flush=True)
            continue
        print(f'Index shard {s:02d} does not exist. Creating ...', flush=True)
        os.makedirs(save_dir, exist_ok=True)
        data_dir = os.path.join(os.getcwd(), f'cc_s{s:02d}') # this is an ephemeral directory
        os.makedirs(data_dir, exist_ok=True)

        remote_data_dir = 's3://ai2-llm/pretraining-data/sources/dclm/pool/documents/CC-MAIN-2025-05/segments'
        output = os.popen(f'aws s3 ls {remote_data_dir}/').read()
        lines = output.split('\n')
        lines = [line for line in lines if 'PRE ' in line]
        subdirs = [line.split('PRE ')[-1].rstrip('/') for line in lines]
        subdirs = sorted(subdirs, key=lambda x: int(x.split('.')[-1]))
        assert len(subdirs) == 100

        for shard_ix in tqdm(list(range(s * 7, min((s + 1) * 7, 100)))):
            subdir = subdirs[shard_ix]
            print(f'Downloading data from s3: subdir={subdir}', flush=True)
            dir = f'{data_dir}/{subdir}'
            os.makedirs(dir, exist_ok=True)
            process = subprocess.Popen(f'aws s3 cp --recursive {remote_data_dir}/{subdir} {dir}', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            process.wait()
            print(f'Downloaded. Total size: {os.popen(f"du -sh {dir}").read()}', flush=True)

        parser = argparse.ArgumentParser()
        parser.add_argument('--doc_sep', type=bytes, default=b'\xff')
        parser.add_argument('--batch_size', type=int, default=65536, help='Batch size for prepare.')
        parser.add_argument('--cpus', type=int, default=mp.cpu_count(), help='Number of CPU cores available to the program.')
        parser.add_argument('--mem', type=int, required=True, help='Amount of memory in GiB available to the program.')
        parser.add_argument('--ulimit', type=int, default=1048576, help='Maximum number of open files allowed.')
        args = parser.parse_args()
        args.data_dir = data_dir
        args.save_dir = save_dir
        args.temp_dir = args.save_dir

        assert args.batch_size > 0
        assert args.cpus > 0

        assert sys.byteorder == 'little'
        resource.setrlimit(resource.RLIMIT_NOFILE, (args.ulimit, args.ulimit))

        prepare(args)
        build_sa_bwt(args, mode='data')
        build_sa_bwt(args, mode='meta')
        print(os.popen(f'./cpp_indexing {args.save_dir} 2>/dev/null').read(), flush=True)

if __name__ == '__main__':
    main()
