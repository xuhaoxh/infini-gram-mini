#!/usr/bin/env bash

set -ex

RUN_NAME="fm_dclm_f10_s1"

gantry run \
  --allow-dirty \
  --name ${RUN_NAME} \
  --task-name ${RUN_NAME} \
  --description ${RUN_NAME} \
  --workspace ai2/infini-llm \
  --budget ai2/oe-training \
  --beaker-image petew/olmo-torch23-gantry \
  --cluster ai2/jupiter-cirrascale-2 \
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
    git checkout dclm; \
    conda install isl=0.12.2 mpc=1.0.3 mpfr=3.1.4; \
    export LD_LIBRARY_PATH=/opt/conda/lib:\$LD_LIBRARY_PATH; \
    conda install psi4::gcc-5=5.2.0; \
    pip install zstandard numpy tqdm; \
    cd src; \
    python indexing.py \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0000.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0001.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0002.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0003.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0004.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0005.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0006.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0007.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0008.json.zst \
      --data_path /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0009.json.zst \
      --save_dir /weka/oe-training-default/jiachengl/hf-fm/index/v2_dclm_f10_s1 \
      --cpus 186 --mem 1860; \
    "
