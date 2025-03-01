import argparse
import gc
import glob
import gzip
import json
import multiprocessing as mp
import numpy as np
import os
import resource
import shutil
import sys
import time
from tqdm import tqdm

HACK = 100000

def load_file(path):
    if path.endswith('.gz'):
        with gzip.open(path, 'rt', encoding='utf-8') as f:
            lines = f.readlines()
    elif path.endswith('.zst'):
        with open(path, 'rb') as f:
            import zstandard as zstd
            dctx = zstd.ZstdDecompressor()
            with dctx.stream_reader(f) as reader:
                decompressed_data = reader.read().decode('utf-8')
            lines = decompressed_data.split('\n')
            if lines[-1] == '':
                lines = lines[:-1]
    elif path.endswith('.jsonl'):
        with open(path, encoding='utf-8') as f:
            lines = f.readlines()
    else:
        raise ValueError(f'Unknown file type: {path}')
    return lines

def parse_line(line, doc_sep, path, linenum):
    meta = json.loads(line.strip('\n'))
    data = meta['text']
    del meta['text']
    data = doc_sep + data.encode('utf-8')
    meta = (json.dumps({'path': path, 'linenum': linenum, 'metadata': meta}) + '\n').encode('utf-8')
    return data, meta

def prepare(args):

    ds_path = os.path.join(args.save_dir, f'data')
    od_path = os.path.join(args.save_dir, f'data_offset')
    mt_path = os.path.join(args.save_dir, f'meta')
    om_path = os.path.join(args.save_dir, f'meta_offset')
    if all([os.path.exists(path) for path in [ds_path, od_path, mt_path, om_path]]):
        print('Step 1 (prepare): Skipped. All files already exist.')
        return

    print('Step 1 (prepare): Starting ...')
    start_time = time.time()

    data_paths = args.data_path
    ds_fout = open(ds_path, 'wb')
    od_fout = open(od_path, 'wb')
    mt_fout = open(mt_path, 'wb')
    om_fout = open(om_path, 'wb')
    with mp.get_context('fork').Pool(args.cpus) as p:
        od = 0
        om = 0
        for data_path in tqdm(data_paths):
            rel_data_path = data_path.split('/')[-1]
            lines = load_file(data_path)
            for offset in tqdm(range(0, len(lines), args.batch_size), total=len(range(0, len(lines), args.batch_size))):
                batch_lines = lines[offset:min(offset+args.batch_size, len(lines))]
                results = p.starmap(parse_line, [(line, args.doc_sep, rel_data_path, offset+i) for i, line in enumerate(batch_lines)])
                for i, (data, meta) in enumerate(results):
                    ds_fout.write(data)
                    od_fout.write(np.array([od], dtype=np.uint64).view(np.uint8).tobytes())
                    od += len(data)

                    mt_fout.write(meta)
                    om_fout.write(np.array([om], dtype=np.uint64).view(np.uint8).tobytes())
                    om += len(meta)
                del results
            del lines
            gc.collect()
    gc.collect()

    ds_fout.write(b'\xfa') # append one zero byte to the end of the file
    mt_fout.write(b'\xfa') # append one zero byte to the end of the file

    ds_fout.close()
    od_fout.close()
    mt_fout.close()
    om_fout.close()

    end_time = time.time()
    print(f'Step 1 (prepare): Done. Took {end_time-start_time:.2f} seconds')

def build_sa_bwt(args, mode):

    ds_path = os.path.join(args.save_dir, mode)
    sa_path = ds_path + '_sa'
    bwt_path = ds_path + '_bwt'
    if os.path.exists(sa_path) and os.path.exists(bwt_path):
        print(f'Step 2 (build_sa_bwt): Skipped. SA and BWT file already exists.')
        return

    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    print('Step 2 (build_sa_bwt): Starting ...')
    start_time_all = time.time()

    # -------- Step 2.1 (make-part) -------- #

    print(f'Step 2.1 (make-part): Starting ...')
    start_time = time.time()

    ds_size = os.path.getsize(ds_path)
    mem_bytes = args.mem * 1024**3
    num_job_batches = 1
    while num_job_batches * (mem_bytes // 8) < ds_size:
        num_job_batches *= 2
    parallel_jobs = args.cpus
    total_jobs = num_job_batches * parallel_jobs
    print(f'Using {num_job_batches} batches of {parallel_jobs} jobs each, for a total of {total_jobs} jobs.')

    S = ds_size // total_jobs

    parts_dir = os.path.join(args.temp_dir, f'parts')
    shutil.rmtree(parts_dir, ignore_errors=True)
    os.makedirs(parts_dir)

    ranges, files = [], []
    for batch_start in tqdm(list(range(0, total_jobs, parallel_jobs))):
        batch_end = min(batch_start+parallel_jobs, total_jobs)
        batch_ranges, batch_files = [], []
        for i in range(batch_start, batch_end):
            s, e = i*S, min((i+1)*S+HACK, ds_size)
            batch_ranges.append((s, e))
            batch_files.append(os.path.join(parts_dir, f'{s}-{e}'))
        ranges += batch_ranges
        files += batch_files
        wait = []
        for (s, e) in batch_ranges:
            cmd = f'./rust_indexing make-part --data-file {ds_path} --parts-dir {parts_dir} --start-byte {s} --end-byte {e}'
            wait.append(os.popen(cmd))
        [x.read() for x in wait]

    end_time = time.time()
    print(f'Step 2.1 (make-part): Done. Took {end_time-start_time:.2f} seconds')

    # -------- Step 2.2 (merge) -------- #

    print(f'Step 2.2 (merge): Starting ...')
    start_time = time.time()

    merged_dir = os.path.join(args.temp_dir, f'merged')
    shutil.rmtree(merged_dir, ignore_errors=True)
    os.makedirs(merged_dir)
    bwt_dir = os.path.join(args.temp_dir, f'bwt')
    shutil.rmtree(bwt_dir, ignore_errors=True)
    os.makedirs(bwt_dir)

    cmd = f'./rust_indexing merge --merged-dir {merged_dir} --bwt-dir {bwt_dir} --suffix-path {" --suffix-path ".join(files)} --num-threads {args.cpus} --hacksize {HACK}'
    pipe = os.popen(cmd)
    output = pipe.read()
    if pipe.close() is not None:
        print('Something went wrong with merging.')
        exit(1)

    shutil.rmtree(parts_dir)

    end_time = time.time()
    print(f'Step 2.2 (merge): Done. Took {end_time-start_time:.2f} seconds')

    # -------- Step 2.3 (concat) -------- #

    print(f'Step 2.3 (concat): Starting ...')
    start_time = time.time()

    os.popen(f'cat {merged_dir}/* > {sa_path}').read()
    shutil.rmtree(merged_dir)
    os.popen(f'cat {bwt_dir}/* > {bwt_path}').read()
    shutil.rmtree(bwt_dir)

    end_time = time.time()
    print(f'Step 2.3 (concat): Done. Took {end_time-start_time:.2f} seconds')

    # -------- Step 2.4 (verify) -------- #

    if not os.path.exists(sa_path):
        print('Failed to create SA')
        exit(1)

    sa_size = os.path.getsize(sa_path)
    if sa_size % ds_size != 0:
        print('SA file size is wrong')
        exit(1)

    end_time_all = time.time()
    print(f'Step 2 (build_sa_bwt): Done. Took {end_time_all-start_time_all:.2f} seconds')

def format_file(args):
    print('Step 3 (format_file): Starting ...')
    start_time = time.time()

    for mode in ['data', 'meta']:
        ds_path = os.path.join(args.save_dir, mode)
        ds_path_new = os.path.join(args.save_dir, f'text_{mode}.sdsl')
        ds_bytes = os.path.getsize(ds_path)
        with open(ds_path_new, 'wb') as ds_fout:
            ds_fout.write(np.array([ds_bytes * 8], dtype=np.uint64).view(np.uint8).tobytes())
        os.popen(f'cat {ds_path} >> {ds_path_new}').read()
        with open(ds_path_new, 'ab') as ds_fout:
            if ds_bytes % 8 != 0:
                ds_fout.write(b'\00' * (8 - ds_bytes % 8))
        os.remove(ds_path)

        sa_path = ds_path + '_sa'
        sa_path_new = os.path.join(args.save_dir, f'sa_{mode}.sdsl')
        sa_bytes = os.path.getsize(sa_path)
        with open(sa_path_new, 'wb') as sa_fout:
            sa_fout.write(np.array([sa_bytes * 8], dtype=np.uint64).view(np.uint8).tobytes())
            sa_fout.write(np.array([sa_bytes // ds_bytes * 8], dtype=np.uint8).tobytes())
        os.popen(f'cat {sa_path} >> {sa_path_new}').read()
        with open(sa_path_new, 'ab') as sa_fout:
            if sa_bytes % 8 != 0:
                sa_fout.write(b'\00' * (8 - sa_bytes % 8))
        os.remove(sa_path)

        bwt_path = ds_path + '_bwt'
        bwt_path_new = os.path.join(args.save_dir, f'bwt_{mode}.sdsl')
        bwt_bytes = os.path.getsize(bwt_path)
        with open(bwt_path_new, 'wb') as bwt_fout:
            bwt_fout.write(np.array([bwt_bytes * 8], dtype=np.uint64).view(np.uint8).tobytes())
        os.popen(f'cat {bwt_path} >> {bwt_path_new}').read()
        with open(bwt_path_new, 'ab') as bwt_fout:
            if bwt_bytes % 8 != 0:
                bwt_fout.write(b'\00' * (8 - bwt_bytes % 8))
        os.remove(bwt_path)

    end_time = time.time()
    print(f'Step 3 (format_file): Done. Took {end_time-start_time:.2f} seconds')

def main():

    parser = argparse.ArgumentParser()
    # parser.add_argument('--data_dir', type=str, required=True, help='Directory containing the raw text corpus. Must be absolute path.')
    parser.add_argument('--data_path', type=str, nargs='+', required=True, help='Path to the raw text corpus. Can be a list of paths. Must be absolute path.')
    parser.add_argument('--temp_dir', type=str, default=None, help='Directory where temporary indexing files are stored. Must be absolute path.')
    parser.add_argument('--save_dir', type=str, required=True, help='Directory where the final index files are stored. Must be absolute path.')
    parser.add_argument('--doc_sep', type=bytes, default=b'\xff')
    parser.add_argument('--batch_size', type=int, default=65536, help='Batch size for prepare.')
    parser.add_argument('--cpus', type=int, default=mp.cpu_count(), help='Number of CPU cores available to the program.')
    parser.add_argument('--mem', type=int, required=True, help='Amount of memory in GiB available to the program.')
    parser.add_argument('--ulimit', type=int, default=1048576, help='Maximum number of open files allowed.')
    args = parser.parse_args()
    if args.temp_dir is None:
        args.temp_dir = args.save_dir
    args.temp_dir = args.temp_dir.rstrip('/')
    args.save_dir = args.save_dir.rstrip('/')

    assert args.batch_size > 0
    assert args.cpus > 0

    assert all([os.path.exists(path) for path in args.data_path])
    os.makedirs(args.temp_dir, exist_ok=True)
    os.makedirs(args.save_dir, exist_ok=True)

    assert sys.byteorder == 'little'
    resource.setrlimit(resource.RLIMIT_NOFILE, (args.ulimit, args.ulimit))

    prepare(args)
    build_sa_bwt(args, mode='data')
    build_sa_bwt(args, mode='meta')
    format_file(args)

    print(os.popen(f'./cpp_indexing {args.save_dir}').read())

if __name__ == '__main__':
    main()