import argparse
import multiprocessing as mp
import os
import resource
import subprocess
import sys
from tqdm import tqdm
from indexing import prepare, build_sa_bwt

def main():

    for s in range(2):
        save_dir = f'/weka/oe-training-default/jiachengl/hf-fm/index/v2_piletrain/{s:01d}'
        if os.path.exists(save_dir):
            print(f'Index shard {s:01d} already exists. Skipping.', flush=True)
            continue
        print(f'Index shard {s:01d} does not exist. Creating ...', flush=True)

        data_dir = os.path.join(os.getcwd(), f'piletrain_s{s:01d}') # this is an ephemeral directory
        os.makedirs(data_dir, exist_ok=True)
        for shard_ix in range(s * 15, (s+1) * 15):
            process = subprocess.Popen(f'cp /weka/oe-training-default/jiachengl/hf-fm/data/piletrain/{shard_ix:02d}.jsonl {data_dir}', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            process.wait()

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

        assert os.path.exists(args.data_dir)
        os.makedirs(args.save_dir, exist_ok=True)

        assert sys.byteorder == 'little'
        resource.setrlimit(resource.RLIMIT_NOFILE, (args.ulimit, args.ulimit))

        prepare(args)
        build_sa_bwt(args, mode='data')
        build_sa_bwt(args, mode='meta')
        print(os.popen(f'./cpp_indexing {args.save_dir} 2>/dev/null').read(), flush=True)

if __name__ == '__main__':
    main()
