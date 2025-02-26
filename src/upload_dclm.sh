#!/usr/bin/env bash

set -ex

RUN_NAME="upload_fm_dclm_f1_all"

gantry run \
  --allow-dirty \
  --name ${RUN_NAME} \
  --task-name ${RUN_NAME} \
  --description ${RUN_NAME} \
  --workspace ai2/infini-llm \
  --budget ai2/oe-training \
  --beaker-image ai2/cuda11.8-ubuntu20.04 \
  --cluster ai2/neptune-cirrascale \
  --cluster ai2/ceres-cirrascale \
  --priority high \
  --no-nfs \
  --weka oe-training-default:/weka/oe-training-default \
  --env-secret GITHUB_TOKEN=GITHUB_TOKEN_HF \
  --no-python \
  --yes \
  -- /bin/bash -c "\
    set -exuo pipefail; \
    IFS=$'\n\t'; \
    for i in {1000..1969}; do scp -o StrictHostKeyChecking=no -i /weka/oe-training-default/jiachengl/hf-fm/fm-index.pem -r /weka/oe-training-default/jiachengl/hf-fm/dclm_f1_all/data_\$(printf '%04d' \$i) ubuntu@44.195.164.244:/data_dclm/dclm_f1_all/; done; \
    "
