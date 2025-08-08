import argparse
import multiprocessing as mp
import os
import resource
import shutil
import subprocess
import sys
from tqdm import tqdm
from indexing import prepare, build_sa_bwt

def main():

    process = subprocess.Popen(f'cp -r /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/starcoder starcoder', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    process.wait()
    # Split starcoder into two roughly equal parts by total file size
    starcoder_dir = 'starcoder'
    starcoder_0_dir = 'olmoe-mix-0924-nodclm_s3/starcoder'
    starcoder_1_dir = 'olmoe-mix-0924-nodclm_s4/starcoder'

    # Get all files and their sizes
    files_with_sizes = []
    for root, _, files in os.walk(starcoder_dir):
        for file in files:
            file_path = os.path.join(root, file)
            size = os.path.getsize(file_path)
            files_with_sizes.append((file_path, size))

    # Sort by filename
    files_with_sizes.sort(key=lambda x: x[0])

    # Split into two groups with similar total sizes
    total_size = sum(size for _, size in files_with_sizes)
    target_size = total_size / 2

    current_size = 0
    split_idx = 0
    for i, (_, size) in enumerate(files_with_sizes):
        current_size += size
        if current_size >= target_size:
            split_idx = i + 1
            break

    # Create directories and move files
    os.makedirs(starcoder_0_dir)
    os.makedirs(starcoder_1_dir)

    for file_path, _ in files_with_sizes[:split_idx]:
        rel_path = os.path.relpath(file_path, starcoder_dir)
        target_dir = os.path.join(starcoder_0_dir, os.path.dirname(rel_path))
        os.makedirs(target_dir, exist_ok=True)
        shutil.move(file_path, os.path.join(starcoder_0_dir, rel_path))

    for file_path, _ in files_with_sizes[split_idx:]:
        rel_path = os.path.relpath(file_path, starcoder_dir)
        target_dir = os.path.join(starcoder_1_dir, os.path.dirname(rel_path))
        os.makedirs(target_dir, exist_ok=True)
        shutil.move(file_path, os.path.join(starcoder_1_dir, rel_path))

    shutil.rmtree(starcoder_dir)


    for s in range(3, 5):
        save_dir = f'/weka/oe-training-default/jiachengl/hf-fm/index/v2_olmoe-mix-0924-nodclm/{s:01d}'
        if os.path.exists(save_dir) and os.path.exists(f'{save_dir}/data.fm9') and os.path.exists(f'{save_dir}/meta.fm9'):
            print(f'Index shard {s:01d} already exists. Skipping.', flush=True)
            continue
        print(f'Index shard {s:01d} does not exist. Creating ...', flush=True)

        data_dir = os.path.join(os.getcwd(), f'olmoe-mix-0924-nodclm_s{s:01d}') # this is an ephemeral directory
        os.makedirs(data_dir, exist_ok=True)

        if s == 0:
            domains = ['pes2o']
        elif s == 1:
            domains = ['starcoder']
        elif s == 2:
            domains = ['algebraic-stack', 'arxiv', 'open-web-math', 'wiki']
        if s < 3:
            for domain in domains:
                process = subprocess.Popen(f'cp -r /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/{domain} {data_dir}/{domain}', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
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

        shutil.rmtree(data_dir)

if __name__ == '__main__':
    main()
