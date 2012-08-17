#!/bin/bash
#PBS -l nodes=1:ppn=12:gb96,walltime=4:00:00
#PBS -j eo
#PBS -q timeshare

module load intel hdf/1.8.8

cd $PBS_O_WORKDIR

export OMP_NUM_THREADS=$PBS_NUM_PPN
export KMP_AFFINITY=granularity=thread,verbose,compact

python benchmem.py

