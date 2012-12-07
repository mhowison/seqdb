#!/bin/bash
#SBATCH -c 12
#SBATCH --mem=90G
#SBATCH -t 2:00:00

ulimit -c unlimited
set -e

export OMP_NUM_THREADS=12
export KMP_AFFINITY="proclist=[12-23]"

FASTQ=/users/mhowison/data/seqdb/SRR493233_1.filt.fastq

N=256
for i in {1..14}; do
	N=$((N*2))
	./benchmem $FASTQ $N
	for t in {1..9}; do
		BENCHMEM_SKIP_ZLIB=1 ./benchmem $FASTQ $N
	done
done

for t in {1..15}; do
	N=256
	for i in {1..14}; do
		N=$((N*2))
		BENCHMEM_SKIP_ZLIB=1 ./benchmem $FASTQ $N
	done
done

