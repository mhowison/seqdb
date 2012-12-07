#!/bin/bash 
#SBATCH -c 12
#SBATCH --mem 90G
#SBATCH -t 12:00:00
#SBATCH -e benchrand-%j.out
#SBATCH -o benchrand-%j.out

export PATH=$PWD:$PATH
TIME="$PWD/time -p"

export OMP_NUM_THREADS=12
export KMP_AFFINITY="proclist=[12-23]"

rm -f /dev/shm/*
set -e
cd /dev/shm

SRC=$HOME/data/seqdb

benchrand() {
  SRA=${1%".fastq"}
  INPUT=/tmp/$1
  SLEN=$2
  ILEN=$3

  echo "SRA=$SRA"
  echo "INPUT=$1"
  echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"
  echo "SEQDB_BLOCKSIZE=$SEQDB_BLOCKSIZE"

  $TIME cp $SRC/$1 $INPUT

  $TIME seqdb-compress $SLEN $ILEN $INPUT 1.seqdb
  $TIME dsrc c $INPUT 1.dsrc

  $TIME benchrand 1.seqdb 1.dsrc 1000
  mv benchrand.ids.txt ~/code/seqdb/bench/benchrand.ids.$SRA.txt
  mv benchrand.seqdb.txt ~/code/seqdb/bench/benchrand.seqdb.$SRA.txt
  mv benchrand.seqdb.fq ~/code/seqdb/bench/benchrand.seqdb.$SRA.fq
  mv benchrand.dsrc.txt ~/code/seqdb/bench/benchrand.dsrc.$SRA.txt
  mv benchrand.dsrc.fq ~/code/seqdb/bench/benchrand.dsrc.$SRA.fq

  rm 1.seqdb 1.dsrc $INPUT
}

benchrand SRR493328.fastq 302 43
benchrand ERR000018.fastq 36 32
benchrand SRR003177.fastq 4398 15
benchrand SRR611141.fastq 406 8
benchrand SRR448020.fastq 172 32
benchrand SRR493233.fastq 200 40

rm -f /dev/shm/*

