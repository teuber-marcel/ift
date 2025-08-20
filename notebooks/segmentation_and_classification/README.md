# Preparing the environment

To install all requisites, run:

1. Navigate into data: `cd data`

2. Install dependencies (conda included): `sh ./install.sh 1`

   2.1 If you have conda already installed, run `sh ./install.sh`

   2.2 If your version of conda cannot be updated, run
       'sudo apt upgrade' and then try it again. 
    
   2.3 It is necessary to replace the variable `CONDA_PATH` (line 30)
       to the real conda path on your machine (e.g.,
       /home/<user>/miniconda3)
    					

To execute the notebooks in seg_parasites and class_citrus, run:

   'conda activate flim'
   'cd seg_parasites'
   'jupyter-lab 1_plain_unet.ipynb'