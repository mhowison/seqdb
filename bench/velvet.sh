#!/bin/bash 
#SBATCH -c 12
#SBATCH --mem 90G
#SBATCH -t 12:00:00
#SBATCH -e velvet-%j.out
#SBATCH -o velvet-%j.out

export PATH=$PWD:$PATH
TIME="$PWD/time -p"

export OMP_NUM_THREADS=12
export KMP_AFFINITY="proclist=[12-23]"

rm -rf /dev/shm/assembly
rm /dev/shm/1.seqdb /dev/shm/1.gz /dev/shm/1.fq
rm /tmp/*.fastq

set -e

cd /dev/shm

SRC=$HOME/data/seqdb

velvet() {
  INPUT=/tmp/$1
  SLEN=$2
  ILEN=$3

  echo "INPUT=$1"
  echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"
  echo "SEQDB_BLOCKSIZE=$SEQDB_BLOCKSIZE"

  $TIME cp $SRC/$1 $INPUT

  velveth assembly 31 -noHash -short -fastq $INPUT
  rm -rf assembly

  $TIME seqdb-compress $SLEN $ILEN $INPUT 1.seqdb
  #seqdb-mount 1.seqdb 1.fq 2>/dev/null &
  mkfifo 1.fq
  seqdb-extract 1.seqdb 1>1.fq 2>/dev/null &
  velveth assembly 31 -noHash -short -fastq 1.fq
  wait
  rm -rf assembly

  $TIME gzip -c $INPUT >1.gz
  velveth assembly 31 -noHash -short -fastq.gz 1.gz
  rm -rf assembly

  rm 1.seqdb 1.gz 1.fq $INPUT
}

velvet SRR493328.fastq 302 43
velvet ERR000018.fastq 36 32
velvet SRR003177.fastq 4398 15
velvet SRR611141.fastq 406 8
velvet SRR448020.fastq 172 32
velvet SRR493233.fastq 200 40

rm -f /dev/shm/*

