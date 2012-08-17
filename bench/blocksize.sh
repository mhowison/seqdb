#!/bin/bash 
#PBS -j eo
#PBS -l nodes=1:ppn=8
#PBS -l walltime=40:00

module load hdf5/1.8.8 intel

export PATH=$HOME/seqdb/bin:$PATH

export OMP_NUM_THREADS=$PBS_NUM_PPN
echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"

export KMP_AFFINITY=granularity=thread,verbose,scatter

cd /dev/shm

SRC=$HOME/data/seqdb

INPUT="ERR000018.filt.fastq"
SLEN=36
ILEN=50

DIFF="diff -q"
STAT="stat -c stat:%s"
USAGE="usage"

BLOCKS="16384 32768 131072 262144 524288 1048576 2097152 4194304 8388608 16777216"

cp $SRC/$INPUT $INPUT

$USAGE cp $INPUT 1
rm 1

for block in $BLOCKS
do
	SEQDB_BLOCKSIZE=$block $USAGE fastq2seqdb -i $INPUT -o 1 -l $SLEN -d $ILEN
	SEQDB_BLOCKSIZE=$block $USAGE seqdb2fastq 1 >2
	$DIFF $INPUT 2
	$STAT 1
	rm 1 2
done

rm $INPUT
