#!/bin/bash 
#PBS -j eo
#PBS -l nodes=1:ppn=12:gb96
#PBS -l walltime=12:00:00
#PBS -q timeshare

module load hdf5/1.8.8

export PATH=$HOME/seqdb/bin:$PATH

export KMP_AFFINITY=granularity=thread,scatter
export GOMP_AFFINITY=0-11
export OMP_NUM_THREADS=$PBS_NUM_PPN

cd /dev/shm

SRC=$HOME/data/seqdb

INPUT="ERR000018.filt.fastq"
SLEN=36
ILEN=50
export SEQDB_BLOCKSIZE=928050

INPUT="SRR493233_1.filt.fastq"
SLEN=100
ILEN=61
export SEQDB_BLOCKSIZE=432251

#INPUT="SRR497004_1.filt.fastq"
#SLEN=51
#ILEN=91
#export SEQDB_BLOCKSIZE=1229250
#
echo "INPUT=$INPUT"
echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"
echo "SEQDB_BLOCKSIZE=$SEQDB_BLOCKSIZE"

DIFF="diff -q"
STAT="stat -c stat:%s"

echo "name:cp"
usage cp $SRC/$INPUT $INPUT
$STAT $INPUT

usage cp $INPUT 1
rm 1

echo "name:reprint"
usage fastq_reprint -l $SLEN -d $ILEN -i $INPUT >1
$DIFF $INPUT 1
$STAT 1
rm 1

usage fastq_reprint -l $SLEN -d $ILEN -i $INPUT >1
rm 1

#echo "name:gzip2"
#usage gzip -2 -c $INPUT >1
#usage gunzip -c 1 >2
#$DIFF $INPUT 2
#$STAT 1
#rm 1 2
#
#echo "name:gzip9"
#usage gzip -9 -c $INPUT >1
#usage gunzip -c 1 >2
#$DIFF $INPUT 2
#$STAT 1
#rm 1 2
#
#echo "name:biohdf"
#usage $HDF5_DIR/bin/bioh5g_import_reads -f 1 -R "/reads" -i $INPUT -F fastq -s $SLEN -d $ILEN
#usage $HDF5_DIR/bin/bioh5g_export_reads -f 1 -R "/reads" -F fastq -o 2
#$DIFF $INPUT 2
#$STAT 1
#rm 1 2

echo "name:biohdf-blosc"
usage $HOME/seqdb/bin/bioh5g_import_reads -f 1 -R "/reads" -i $INPUT -F fastq -s $SLEN -d $ILEN -Z 9
usage $HOME/seqdb/bin/bioh5g_export_reads -f 1 -R "/reads" -F fastq -o 2
$DIFF $INPUT 2
$STAT 1
rm 1 2

#echo "name:seqdb"
#usage fastq2seqdb -i $INPUT -o 1 -l $SLEN -d $ILEN
#usage seqdb2fastq 1 >2
#$DIFF $INPUT 2
#$STAT 1
#rm 1 2
#
#echo "name:pytables"
#usage fastq2tables.py $INPUT 1 $SLEN $ILEN
#usage tables2fastq.py 1 >2
#$DIFF $INPUT 2
#$STAT 1
#rm 1 2
#
#echo "name:dsrc"
#usage dsrc c $INPUT 1
#usage dsrc d 1 2
#$DIFF $INPUT 2
#$STAT 1
#rm 1 2
#
#echo "name:beetl"
#usage Beetl bcr -i $INPUT -o 1 -m 0
#usage Beetl bcr -i 1 -o 2 -m 0
#$DIFF $INPUT 2
#$STAT 1
#rm 1 2

#echo "name:encseq"
#usage gt encseq encode $INPUT
#usage gt encseq decode $INPUT >2
#$DIFF $INPUT 2

rm $INPUT

