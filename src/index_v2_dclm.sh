#!/usr/bin/env bash

set -ex

RUN_NAME="fm_v2_dclm_all"

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
  --env-secret AWS_ACCESS_KEY_ID=AWS_ACCESS_KEY_ID \
  --env-secret AWS_SECRET_ACCESS_KEY=AWS_SECRET_ACCESS_KEY \
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
    pip install zstandard numpy tqdm awscli; \
    cd src; \
    python indexing.py --cpus 186 --mem 1860; \
    "
