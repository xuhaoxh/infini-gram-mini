import argparse
import multiprocessing as mp
import os
import resource
import subprocess
import sys
from tqdm import tqdm
from indexing import prepare, build_sa_bwt

def main():

    save_dir = f'/data/jiachengl/hf-fm/index/v2_pileval'
    if os.path.exists(save_dir) and os.path.exists(f'{save_dir}/data.fm9') and os.path.exists(f'{save_dir}/meta.fm9'):
        print(f'Index already exists. Skipping.', flush=True)
        return
    print(f'Index does not exist. Creating ...', flush=True)

    data_dir = f'/data/jiachengl/ha-infini-gram/data/pileval'

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
    print(os.popen(f'./cpp_indexing {args.save_dir}').read(), flush=True)

if __name__ == '__main__':
    main()
