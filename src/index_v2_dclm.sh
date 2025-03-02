#!/usr/bin/env bash

set -ex

RUN_NAME="fm_v2_dclm_f80_s1"

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
      --data_path \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0000.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0001.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0002.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0003.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0004.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0005.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0006.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0007.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0008.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0009.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0010.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0011.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0012.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0013.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0014.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0015.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0016.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0017.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0018.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0019.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0020.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0021.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0022.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0023.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0024.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0025.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0026.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0027.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0028.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0029.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0030.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0031.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0032.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0033.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0034.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0035.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0036.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0037.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0038.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0039.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0040.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0041.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0042.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0043.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0044.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0045.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0046.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0047.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0048.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0049.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0050.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0051.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0052.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0053.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0054.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0055.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0056.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0057.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0058.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0059.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0060.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0061.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0062.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0063.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0064.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0065.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0066.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0067.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0068.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0069.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0070.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0071.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0072.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0073.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0074.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0075.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0076.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0077.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0078.json.zst \
        /weka/oe-data-default/ai2-llm/pretraining-data/sources/olmo-mix/olmoe-mix-0924/data/dclm/dclm-0079.json.zst \
      --save_dir /weka/oe-training-default/jiachengl/hf-fm/index/v2_dclm_f80_s1 \
      --cpus 186 --mem 1860; \
    "
