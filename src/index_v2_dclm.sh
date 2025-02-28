#!/usr/bin/env bash

set -ex

RUN_NAME="fm_dclm_f100_s1"

gantry run \
  --allow-dirty \
  --name ${RUN_NAME} \
  --task-name ${RUN_NAME} \
  --description ${RUN_NAME} \
  --workspace ai2/infini-llm \
  --budget ai2/oe-training \
  --beaker-image petew/olmo-torch23-gantry \
  --cluster ai2/neptune-cirrascale \
  --cluster ai2/ceres-cirrascale \
  --priority high \
  --no-nfs \
  --weka oe-training-default:/weka/oe-training-default \
  --weka oe-data-default:/weka/oe-data-default \
  --cpus 186 \
  --memory 1912GiB \
  --shared-memory 10GiB \
  --env-secret GITHUB_TOKEN=GITHUB_TOKEN_HF \
  --no-python \
  --yes \
  -- /bin/bash -c "\
    set -exuo pipefail; \
    IFS=$'\n\t'; \
    conda shell.bash activate base; \
    conda install isl=0.12.2 mpc=1.0.3 mpfr=3.1.4; \
    export LD_LIBRARY_PATH=/data/jiachengl/miniconda3/envs/hf-parallel-sdsl/lib; \ # this is problematic
    # install rust and compile binary
    conda install psi4::gcc-5=5.2.0; \
    pip install zstandard numpy tqdm; \
    git checkout dclm; \
    cd src; \
    GCILK=true g++ -std=c++11 -I../parallel_sdsl/include -L../parallel_sdsl/lib indexing.cpp -o cpp_indexing -lsdsl -ldivsufsort -ldivsufsort64 -DCILKP -fcilkplus -O2; \
    python indexing.py \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0000.json.zst \ # add more!
      --save_dir /weka/oe-training-default/jiachengl/hf-fm/v2_dclm_f100_s1 \
      --cpus 180 --mem 1800; \
    "
