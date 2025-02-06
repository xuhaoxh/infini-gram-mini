#!/usr/bin/env bash

set -ex

RUN_NAME="fm_dclm_f1_all"

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
    conda install gxx=11.2.0 -c conda-forge; \
    pip install zstandard; \
    cd src; \
    g++ -std=c++17 -I../sdsl/include -L../sdsl/lib construct.cpp -o construct -lsdsl -ldivsufsort -ldivsufsort64; \
    python index_dclm.py; \
    "
