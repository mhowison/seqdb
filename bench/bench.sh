#!/bin/bash 
#SBATCH -c 12
#SBATCH --mem 90G
#SBATCH -t 24:00:00
#SBATCH -e bench-%j.out
#SBATCH -o bench-%j.out

#set -e

export PATH=$PWD:$PATH
TIME="$PWD/time -p"

export OMP_NUM_THREADS=12
export KMP_AFFINITY="proclist=[12-23]"

cd /dev/shm
rm -f *

SRC=$HOME/data/seqdb

bench() {
  INPUT=/tmp/$1
  SLEN=$2
  ILEN=$3

  echo "INPUT=$1"
  echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"
  echo "SEQDB_BLOCKSIZE=$SEQDB_BLOCKSIZE"

  DIFF="diff -q"
  STAT="stat -c stat:%s"

  echo "name:cp"
  $TIME cp $SRC/$1 $INPUT
  $STAT $INPUT

  $TIME cp $INPUT 1
  rm 1

  echo "name:seqdb"
  $TIME seqdb-compress $SLEN $ILEN $INPUT 1
  $TIME seqdb-extract 1 >2
  $DIFF $INPUT 2
  $STAT 1
  rm 1 2

  echo "name:dsrc"
  $TIME dsrc c $INPUT 1
  $TIME dsrc d 1 2
  $DIFF $INPUT 2
  $STAT 1
  rm 1 2

  echo "name:biohdf"
  $TIME bioh5g_import_reads -f 1 -R "/reads" -i $INPUT -F fastq -s $SLEN -d $ILEN
  $TIME bioh5g_export_reads -f 1 -R "/reads" -F fastq -o 2
  $DIFF $INPUT 2
  $STAT 1
  rm 1 2

  echo "name:biohdf-blosc"
  $TIME bioh5g_import_reads -f 1 -R "/reads" -i $INPUT -F fastq -s $SLEN -d $ILEN -Z 4
  $TIME bioh5g_export_reads -f 1 -R "/reads" -F fastq -o 2
  $DIFF $INPUT 2
  $STAT 1
  rm 1 2

  echo "name:gzip"
  $TIME gzip -c $INPUT >1
  $TIME gunzip -c 1 >2
  $DIFF $INPUT 2
  $STAT 1
  rm 1 2

  rm $INPUT
}

bench SRR493328.fastq 302 43
bench ERR000018.fastq 36 32
bench SRR003177.fastq 4398 15
bench SRR611141.fastq 406 8
bench SRR448020.fastq 172 32
bench SRR493233.fastq 200 40

rm -f /dev/shm/*

