SeqDB
=====

SeqDB is a file format, compressor and storage tool for the raw data produced
by Next-Generation Sequencing platforms like the like the Illumina HiSeq 2000
and MiSeq and the Life Technologies Ion Torrent PGM.

SeqDB offers high-throughput compression of sequence data with similar
compression ratios to gzip.  It achieves this by combining the existing
multi-threaded Blosc compressor with a new data-parallel byte-packing scheme,
called SeqPack, which interleaves sequence data and quality scores.

For help with configuring, building, and installing SeqDB, see INSTALL.

For an example of how to use SeqDB, see the TUTORIAL.

If you plan to use SeqDB as a long term archival format for your data, we
strongly recommend you verify the FASTQ output from SeqDB against your
original FASTQ file. This is easy to do in UNIX with:

    $ seqdb extract my.seqdb | diff -q - original.fastq

If this comand returns:

    Files my.seqdb and original.fastq differ

then the data was not stored correctly, possible because of some of the issues
described next. Otherwise, your SeqDB file is good.

In general, you should make sure that your input FASTQ meets the requirements
of SeqDB:

* The input sequences can only have the characters A, T, C, G or N. Any other
  character will not be stored correctly, e.g. you must convert lower-case to
  upper-case.
* The quality scores must be in encoded as
  [Phred+33](http://en.wikipedia.org/wiki/FASTQ_format#Encoding).
* The quality scores must lie in the range from 0 to 50 (Phred+33 characters
  '!' through 'S'). For Illumina data in the CASAVA 1.8 format, the range is
  already limited to 0 to 41.

WARNING for Ion Torrent data: there is a known bug with Torrent Suite v3.0
that generates suspiciously high quality scores (62) at the start of reads that
begin with a G base. This lies outside of the quality score range that SeqDB
can correctly store, and you need to first sanitize the data with a command
like:

    $ sed 's/^_/H/' < dodgy.fastq > fixed.fastq

More information is available from this
[blog](http://thegenomefactory.blogspot.com/2012/09/problematic-fastq-output-from-ion.html).

SeqDB version 0.1.X can store variable-length IDs, but it doesn't correctly
store variable-length sequences.

SeqDB is not designed to handle color space sequences, e.g. the CSFASTQ format
for storing AB SOLiD data.

Authors
-------

Mark Howison <mhowison@brown.edu>

For support or bug reports, please create an issue at GitHub:

https://github.com/mhowison/seqdb/issues

Citing
------

M. Howison, "High-throughput compression of FASTQ data with SeqDB,"
IEEE/ACM Transactions on Computational Biology and Bioinformatics, (in press).
[pdf](http://ccv.brown.edu/mhowison/Howison_SeqDB_TCCB_2012.pdf)

License
-------

Copyright 2011-2012, Brown University, Providence, RI. All Rights Reserved.

See LICENSES/SEQDB.txt for full terms of use.

SeqDB includes source code from BLOSC 1.1.5 (see LICENSES/BLOSC.txt,
LICENSES/FASTLZ.txt, and LICENSES/H5PY.txt for full terms of use).


