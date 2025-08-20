#!/bin/bash

echo "[INFO] Installing Requisites"
sudo apt-get update
DEBIAN_FRONTEND=noninteractive sudo apt-get install -y \
 wget ffmpeg libsm6 libxext6 liblapacke.so.3

if [ $# -gt 0 ]
  then
    if [ $1 -eq 1 ]
        then
            echo "[INFO] Installing miniconda!"
            CONDA_VERSION=py311_23.5.2-0
            CONDA_SHA256=634d76df5e489c44ade4085552b97bebc786d49245ed1a830022b0b406de5817
            
            # Installs Miniconda
            wget --quiet https://repo.anaconda.com/miniconda/Miniconda3-${CONDA_VERSION}-Linux-x86_64.sh -O miniconda.sh && \
            echo "${CONDA_SHA256} miniconda.sh" >  miniconda.md5 && \
            if ! sha256sum --check --status -c miniconda.md5; then exit 1; fi && \
            sh miniconda.sh -b -f -p /opt/conda && \
            rm miniconda.sh miniconda.md5 && \
            ln -s /opt/conda/etc/profile.d/conda.sh /etc/profile.d/conda.sh && \
            echo "/opt/conda/etc/profile.d/conda.sh" >> ~root/.bashrc && \
            export PATH="/opt/conda/bin/:$PATH" && \
            source ~root/.bashrc && \
            echo "[INFO] Miniconda installed!"
        fi
fi

CONDA_PATH=/home/afalcao/miniconda3
#Example: CONDA_PATH="/home/afalcao/miniconda3"

echo "[INFO] Creating Environment!"
# Updates conda
"${CONDA_PATH}"/bin/conda update -y -n base conda &&\
"${CONDA_PATH}"/bin/conda update -y --all &&\
# Creates flim environment
"${CONDA_PATH}"/bin/conda env create -y -f environment.yml

echo "[INFO] Environment Created!"
