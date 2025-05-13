import argparse
import multiprocessing as mp
import numpy as np
import os
import resource
import shutil
import subprocess
import sys
from tqdm import tqdm
from indexing import prepare, build_sa_bwt

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('--crawl_name', type=str, required=True)
    parser.add_argument('--shards_per_shard', type=int, default=7)
    parser.add_argument('--doc_sep', type=bytes, default=b'\xff')
    parser.add_argument('--batch_size', type=int, default=65536, help='Batch size for prepare.')
    parser.add_argument('--cpus', type=int, default=mp.cpu_count(), help='Number of CPU cores available to the program.')
    parser.add_argument('--mem', type=int, required=True, help='Amount of memory in GiB available to the program.')
    parser.add_argument('--ulimit', type=int, default=1048576, help='Maximum number of open files allowed.')
    args = parser.parse_args()

    assert args.batch_size > 0
    assert args.cpus > 0

    assert sys.byteorder == 'little'
    resource.setrlimit(resource.RLIMIT_NOFILE, (args.ulimit, args.ulimit))

    args.num_shards = int(np.ceil(100 / args.shards_per_shard))
    for s in range(args.num_shards):
        save_dir = f'/data_t/v2_cc-{args.crawl_name}/{s:02d}'
        os.makedirs(save_dir, exist_ok=True)
        data_dir = f'/data_c/cc-{args.crawl_name}/{s:02d}'
        os.makedirs(data_dir, exist_ok=True)

        args.data_dir = data_dir
        args.save_dir = save_dir
        args.temp_dir = save_dir

        remote_data_dir = f's3://ai2-llm/pretraining-data/sources/dclm/pool/documents/CC-MAIN-{args.crawl_name}/segments'
        output = os.popen(f'aws s3 ls {remote_data_dir}/').read()
        lines = output.split('\n')
        lines = [line for line in lines if 'PRE ' in line]
        subdirs = [line.split('PRE ')[-1].rstrip('/') for line in lines]
        subdirs = sorted(subdirs, key=lambda x: int(x.split('.')[-1]))
        assert len(subdirs) == 100

        for shard_ix in tqdm(list(range(s * args.shards_per_shard, min((s + 1) * args.shards_per_shard, 100)))):
            subdir = subdirs[shard_ix]
            print(f'Downloading data from s3: subdir={subdir}', flush=True)
            dir = f'{data_dir}/{subdir}'
            os.makedirs(dir, exist_ok=True)
            process = subprocess.Popen(f'aws s3 cp --recursive {remote_data_dir}/{subdir} {dir}', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            process.wait()
            print(f'Downloaded. Total size: {os.popen(f"du -sh {dir}").read()}', flush=True)

        prepare(args)
        build_sa_bwt(args, mode='data')
        build_sa_bwt(args, mode='meta')
        print(os.popen(f'./cpp_indexing {args.save_dir} 2>/dev/null').read(), flush=True)

        shutil.rmtree(data_dir)
        shutil.move(save_dir, f'/data_i/v2_cc-{args.crawl_name}/{s:02d}')

if __name__ == '__main__':
    main()
