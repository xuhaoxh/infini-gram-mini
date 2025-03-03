import argparse
import multiprocessing as mp
import os
import resource
import subprocess
import sys
from tqdm import tqdm
from indexing import prepare, build_sa_bwt, format_file

def main():

    for s in range(25):
        save_dir = f'/weka/oe-training-default/jiachengl/hf-fm/index/v2_dclm_all/{s:02d}'
        if os.path.exists(save_dir):
            print(f'Index shard {s:02d} already exists. Skipping.')
            continue
        print(f'Index shard {s:02d} does not exist. Creating ...')
        os.makedirs(save_dir, exist_ok=True)
        data_dir = os.path.join(os.getcwd(), f'dclm_s{s:02d}') # this is an ephemeral directory
        os.makedirs(data_dir, exist_ok=True)

        for shard_ix in tqdm(list(range(s * 4, (s + 1) * 4))):
            global_shard_ix = shard_ix // 10 + 1
            local_shard_ix = shard_ix % 10
            print(f'Downloading data from s3: global_shard_ix={global_shard_ix}, local_shard_ix={local_shard_ix}')
            dir = f'{data_dir}/global-shard_{global_shard_ix:02d}_of_10/local-shard_{local_shard_ix:02d}_of_10'
            os.makedirs(dir, exist_ok=True)
            process = subprocess.Popen(f'aws s3 cp --recursive s3://ai2-llm/pretraining-data/sources/dclm/baseline/documents/global-shard_{global_shard_ix:02d}_of_10/local-shard_{local_shard_ix:01d}_of_10 {dir}', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            process.wait()
            print(f'Downloaded. Total size: {os.popen(f"du -sh {dir}").read()}')

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
        format_file(args)
        print(os.popen(f'./cpp_indexing {args.save_dir}').read())

if __name__ == '__main__':
    main()
