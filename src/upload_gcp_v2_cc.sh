#!/usr/bin/env bash

set -ex

RUN_NAME="upload_gcp_fm_v2_cc"

gantry run \
  --allow-dirty \
  --name ${RUN_NAME} \
  --task-name ${RUN_NAME} \
  --description ${RUN_NAME} \
  --workspace ai2/fm-index \
  --budget ai2/oe-training \
  --beaker-image ai2/cuda11.8-ubuntu20.04 \
  --cluster ai2/neptune-cirrascale \
  --cluster ai2/ceres-cirrascale \
  --priority normal \
  --not-preemptible \
  --weka oe-training-default:/weka/oe-training-default \
  --env-secret GITHUB_TOKEN=GITHUB_TOKEN \
  --no-python \
  --yes \
  -- /bin/bash -c "\
    set -exuo pipefail; \
    IFS=$'\n\t'; \
    for i in {0..14}; do scp -o StrictHostKeyChecking=no -i /weka/oe-training-default/jiachengl/.ssh/id_rsa -r /weka/oe-training-default/jiachengl/hf-fm/index/v2_cc/2025-05/\$(printf '%02d' \$i) jiachengl@34.83.121.70:/data_v2_cc-2025-05/v2_cc-2025-05/; done; \
    "
