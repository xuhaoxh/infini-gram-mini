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
    elif path.endswith('.zst') or path.endswith('.zstd'):
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

def parse_line(line, doc_sep, rel_path, linenum):
    meta = json.loads(line.strip('\n'))
    data = meta['text']
    del meta['text']
    data = doc_sep + data.encode('utf-8')
    meta = (json.dumps({'path': rel_path, 'linenum': linenum, 'metadata': meta}) + '\n').encode('utf-8')
    return data, meta

def prepare_fewfiles(args):

    ds_path = os.path.join(args.save_dir, f'text_data.sdsl')
    od_path = os.path.join(args.save_dir, f'data_offset')
    mt_path = os.path.join(args.save_dir, f'text_meta.sdsl')
    om_path = os.path.join(args.save_dir, f'meta_offset')
    if all([os.path.exists(path) for path in [ds_path, od_path, mt_path, om_path]]):
        print('Step 1 (prepare): Skipped. All files already exist.', flush=True)
        return

    print('Step 1 (prepare): Starting ...', flush=True)
    start_time = time.time()

    data_paths = glob.glob(f'{args.data_dir}/**/*.json*', recursive=True)
    data_paths = list(sorted(data_paths))
    ds_fout = open(ds_path, 'wb')
    od_fout = open(od_path, 'wb')
    mt_fout = open(mt_path, 'wb')
    om_fout = open(om_path, 'wb')

    # write a placeholder header
    ds_fout.write(np.array([0], dtype=np.uint64).view(np.uint8).tobytes())
    mt_fout.write(np.array([0], dtype=np.uint64).view(np.uint8).tobytes())

    with mp.get_context('fork').Pool(args.cpus) as p:
        od = 0
        om = 0
        for data_path in data_paths:
            rel_data_path = data_path[len(args.data_dir)+1:]
            lines = load_file(data_path)
            for offset in range(0, len(lines), args.batch_size):
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

    # append one zero byte to the end of the file, pad to 8 bytes, and overwrite the header
    ds_fout.write(b'\xfa')
    ds_size = ds_fout.tell() - 8
    if ds_size % 8 != 0:
        ds_fout.write(b'\00' * (8 - ds_size % 8))
    ds_fout.seek(0)
    ds_fout.write(np.array([ds_size * 8], dtype=np.uint64).view(np.uint8).tobytes())
    mt_fout.write(b'\xfa')
    mt_size = mt_fout.tell() - 8
    if mt_size % 8 != 0:
        mt_fout.write(b'\00' * (8 - mt_size % 8))
    mt_fout.seek(0)
    mt_fout.write(np.array([mt_size * 8], dtype=np.uint64).view(np.uint8).tobytes())

    ds_fout.close()
    od_fout.close()
    mt_fout.close()
    om_fout.close()

    end_time = time.time()
    print(f'Step 1 (prepare): Done. Took {end_time-start_time:.2f} seconds', flush=True)

def prepare_manyfiles_map(filenum, path, temp_dir, doc_sep, rel_path):
    lines = load_file(path)

    ds_fout = open(f'{temp_dir}/files/text_data.{filenum:04d}', 'wb')
    od_fout = open(f'{temp_dir}/files/data_offset.{filenum:04d}', 'wb')
    mt_fout = open(f'{temp_dir}/files/text_meta.{filenum:04d}', 'wb')
    om_fout = open(f'{temp_dir}/files/meta_offset.{filenum:04d}', 'wb')
    od = 0
    om = 0

    for linenum, line in enumerate(lines):
        data, meta = parse_line(line, doc_sep, rel_path, linenum)
        ds_fout.write(data)
        od_fout.write(np.array([od], dtype=np.uint64).view(np.uint8).tobytes())
        od += len(data)
        mt_fout.write(meta)
        om_fout.write(np.array([om], dtype=np.uint64).view(np.uint8).tobytes())
        om += len(meta)

    ds_fout.close()
    od_fout.close()
    mt_fout.close()
    om_fout.close()

def prepare_manyfiles_reduce(filenum, data_prev_bytes, meta_prev_bytes, offset_prev_bytes, temp_dir, save_dir):
    for mode in ['data', 'meta']:
        text_fout = open(f'{save_dir}/text_{mode}.sdsl', 'rb+') # do not truncate
        offset_fout = open(f'{save_dir}/{mode}_offset', 'rb+')
        prev_bytes = data_prev_bytes if mode == 'data' else meta_prev_bytes
        text_fout.seek(8 + prev_bytes)
        offset_fout.seek(offset_prev_bytes)

        with open(f'{temp_dir}/files/text_{mode}.{filenum:04d}', 'rb') as f:
            text_fout.write(f.read())
        with open(f'{temp_dir}/files/{mode}_offset.{filenum:04d}', 'rb') as f:
            offsets = np.frombuffer(f.read(), dtype=np.uint8).view(np.uint64).copy()
            offsets += prev_bytes
            offset_fout.write(offsets.view(np.uint8).tobytes())

        text_fout.close()
        offset_fout.close()

def prepare_manyfiles(args):

    ds_path = os.path.join(args.save_dir, f'text_data.sdsl')
    od_path = os.path.join(args.save_dir, f'data_offset')
    mt_path = os.path.join(args.save_dir, f'text_meta.sdsl')
    om_path = os.path.join(args.save_dir, f'meta_offset')
    if all([os.path.exists(path) for path in [ds_path, od_path, mt_path, om_path]]):
        print('Step 1 (prepare): Skipped. All files already exist.', flush=True)
        return

    print('Step 1 (prepare): Starting ...', flush=True)
    start_time = time.time()

    data_paths = glob.glob(f'{args.data_dir}/**/*.json*', recursive=True)
    data_paths = list(sorted(data_paths))

    with mp.get_context('fork').Pool(args.cpus) as p:
        shutil.rmtree(f'{args.temp_dir}/files', ignore_errors=True)
        os.makedirs(f'{args.temp_dir}/files')
        _ = p.starmap(prepare_manyfiles_map, [(filenum, data_path, args.temp_dir, args.doc_sep, data_path[len(args.data_dir)+1:]) for (filenum, data_path) in enumerate(data_paths)])

        tasks = []
        data_prev_bytes = 0
        meta_prev_bytes = 0
        offset_prev_bytes = 0
        for filenum in range(len(data_paths)):
            tasks.append((filenum, data_prev_bytes, meta_prev_bytes, offset_prev_bytes, args.temp_dir, args.save_dir))
            data_prev_bytes += os.path.getsize(f'{args.temp_dir}/files/text_data.{filenum:04d}')
            meta_prev_bytes += os.path.getsize(f'{args.temp_dir}/files/text_meta.{filenum:04d}')
            offset_prev_bytes += os.path.getsize(f'{args.temp_dir}/files/data_offset.{filenum:04d}')

        with open(ds_path, 'wb') as f:
            data_file_size = 8 + data_prev_bytes + (8 - data_prev_bytes % 8) # \xfa + padding
            f.truncate(data_file_size)
            f.seek(0)
            f.write(np.array([(data_prev_bytes + 1) * 8], dtype=np.uint64).view(np.uint8).tobytes())
            f.seek(8 + data_prev_bytes)
            f.write(b'\xfa')
            f.write(b'\00' * (8 - data_prev_bytes % 8 - 1))
        with open(od_path, 'wb') as f:
            data_offset_file_size = offset_prev_bytes
            f.truncate(data_offset_file_size)
        with open(mt_path, 'wb') as f:
            meta_file_size = 8 + meta_prev_bytes + (8 - meta_prev_bytes % 8) # \xfa + padding
            f.truncate(meta_file_size)
            f.seek(0)
            f.write(np.array([(meta_prev_bytes + 1) * 8], dtype=np.uint64).view(np.uint8).tobytes())
            f.seek(8 + meta_prev_bytes)
            f.write(b'\xfa')
            f.write(b'\00' * (8 - meta_prev_bytes % 8 - 1))
        with open(om_path, 'wb') as f:
            meta_offset_file_size = offset_prev_bytes
            f.truncate(meta_offset_file_size)

        _ = p.starmap(prepare_manyfiles_reduce, tasks)
        shutil.rmtree(f'{args.temp_dir}/files')

    end_time = time.time()
    print(f'Step 1 (prepare): Done. Took {end_time-start_time:.2f} seconds', flush=True)

def prepare(args):
    data_paths = glob.glob(f'{args.data_dir}/**/*.json*', recursive=True)
    if len(data_paths) <= 50:
        prepare_fewfiles(args)
    else:
        prepare_manyfiles(args)

def build_sa_bwt(args, mode):

    ds_path = os.path.join(args.save_dir, f'text_{mode}.sdsl')
    sa_path = os.path.join(args.save_dir, f'sa_{mode}.sdsl')
    bwt_path = os.path.join(args.save_dir, f'bwt_{mode}.sdsl')
    if all(os.path.exists(path) for path in [sa_path, bwt_path]):
        print(f'Step 2 (build_sa_bwt): Skipped. SDSL files already exists.', flush=True)
        return

    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    print('Step 2 (build_sa_bwt): Starting ...', flush=True)
    start_time_all = time.time()

    # -------- Step 2.1 (make-part) -------- #

    print(f'\tStep 2.1 (make-part): Starting ...', flush=True)
    start_time = time.time()

    DS_OFFSET = 8
    with open(ds_path, 'rb') as f:
        ds_size = int.from_bytes(f.read(8), 'little') // 8
    ratio = int(np.ceil(np.log2(ds_size) / 8))
    mem_bytes = args.mem * 1024**3
    num_job_batches = 1
    while num_job_batches * (mem_bytes // 12) < ds_size:
        num_job_batches *= 2
    parallel_jobs = args.cpus
    total_jobs = num_job_batches * parallel_jobs
    # print(f'Using {num_job_batches} batches of {parallel_jobs} jobs each, for a total of {total_jobs} jobs.', flush=True)

    S = ds_size // total_jobs

    parts_dir = os.path.join(args.temp_dir, f'parts')
    shutil.rmtree(parts_dir, ignore_errors=True)
    os.makedirs(parts_dir)

    for batch_start in range(0, total_jobs, parallel_jobs):
        batch_end = min(batch_start+parallel_jobs, total_jobs)
        batch_ranges = []
        for i in range(batch_start, batch_end):
            s, e = DS_OFFSET + i*S, DS_OFFSET + min((i+1)*S+HACK, ds_size)
            batch_ranges.append((s, e))
        pipes = []
        for (s, e) in batch_ranges:
            pipes.append(os.popen(f'./rust_indexing make-part --data-file {ds_path} --parts-dir {parts_dir} --start-byte {s} --end-byte {e} --ratio {ratio}'))
        [pipe.read() for pipe in pipes]
        if any([pipe.close() is not None for pipe in pipes]):
            print('\tStep 2.1 (make-part): Something went wrong', flush=True)
            exit(1)

    end_time = time.time()
    print(f'\tStep 2.1 (make-part): Done. Took {end_time-start_time:.2f} seconds', flush=True)

    # -------- Step 2.2 (merge) -------- #

    print(f'\tStep 2.2 (merge): Starting ...', flush=True)
    start_time = time.time()

    merged_dir = os.path.join(args.temp_dir, f'merged')
    shutil.rmtree(merged_dir, ignore_errors=True)
    os.makedirs(merged_dir)
    bwt_dir = os.path.join(args.temp_dir, f'bwt')
    shutil.rmtree(bwt_dir, ignore_errors=True)
    os.makedirs(bwt_dir)

    pipe = os.popen(f'./rust_indexing merge --data-file {ds_path} --parts-dir {parts_dir} --merged-dir {merged_dir} --bwt-dir {bwt_dir} --num-threads {args.cpus} --hacksize {HACK} --ratio {ratio}')
    pipe.read()
    if pipe.close() is not None:
        print('\tStep 2.2 (merge): Something went wrong', flush=True)
        exit(1)

    shutil.rmtree(parts_dir)

    end_time = time.time()
    print(f'\tStep 2.2 (merge): Done. Took {end_time-start_time:.2f} seconds', flush=True)

    # -------- Step 2.3 (concat) -------- #

    print(f'\tStep 2.3 (concat): Starting ...', flush=True)
    start_time = time.time()

    pipe = os.popen(f'./rust_indexing concat --data-file {ds_path} --merged-dir {merged_dir} --merged-file {sa_path} --bwt-dir {bwt_dir} --bwt-file {bwt_path} --num-threads {args.cpus} --ratio {ratio}')
    pipe.read()
    if pipe.close() is not None:
        print('\tStep 2.3 (concat): Something went wrong', flush=True)
        exit(1)

    shutil.rmtree(merged_dir)
    shutil.rmtree(bwt_dir)

    end_time = time.time()
    print(f'\tStep 2.3 (concat): Done. Took {end_time-start_time:.2f} seconds', flush=True)

    end_time_all = time.time()
    print(f'Step 2 (build_sa_bwt): Done. Took {end_time_all-start_time_all:.2f} seconds', flush=True)

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('--data_dir', type=str, required=True, help='Directory containing the raw text corpus. Must be absolute path.')
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
    args.data_dir = args.data_dir.rstrip('/')
    args.temp_dir = args.temp_dir.rstrip('/')
    args.save_dir = args.save_dir.rstrip('/')

    assert args.batch_size > 0
    assert args.cpus > 0

    assert os.path.exists(args.data_dir)
    os.makedirs(args.temp_dir, exist_ok=True)
    os.makedirs(args.save_dir, exist_ok=True)

    assert sys.byteorder == 'little'
    resource.setrlimit(resource.RLIMIT_NOFILE, (args.ulimit, args.ulimit))

    prepare(args)
    build_sa_bwt(args, mode='data')
    build_sa_bwt(args, mode='meta')
    print(os.popen(f'./cpp_indexing {args.save_dir} 2>/dev/null').read(), flush=True)

if __name__ == '__main__':
    main()