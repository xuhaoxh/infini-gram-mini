#!/usr/bin/env bash

set -ex

RUN_NAME="upload_fm_v2_dclm_all"

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
  --not-preemptible \
  --weka oe-training-default:/weka/oe-training-default \
  --env-secret GITHUB_TOKEN=GITHUB_TOKEN_HF \
  --no-python \
  --yes \
  -- /bin/bash -c "\
    set -exuo pipefail; \
    IFS=$'\n\t'; \
    for i in {0..24}; do scp -o StrictHostKeyChecking=no -i /weka/oe-training-default/jiachengl/hf-fm/fm-index-ai2.pem -r /weka/oe-training-default/jiachengl/hf-fm/index/v2_dclm_all/\$(printf '%02d' \$i) ubuntu@3.95.52.164:/data_v2_dclm/v2_dclm_all/; done; \
    "
