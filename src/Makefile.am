lib_LIBRARIES = libseqdb.a
bin_PROGRAMS = seqdb-compress seqdb-extract seqdb-profile

libseqdb_a_SOURCES = \
	seq.cpp seqpack.cpp seqdb.cpp h5seqdb.cpp fastq.cpp \
	blosc_filter.c blosc.c blosclz.c shuffle.c \
	seq.h seqpack.h seqdb.h h5seqdb.h fastq.h util.h \
	blosc_filter.h blosc.h blosclz.h shuffle.h

LDADD = libseqdb.a

seqdb_compress_SOURCES = seqdb-compress.cpp
seqdb_extract_SOURCES = seqdb-extract.cpp
seqdb_profile_SOURCES = seqdb-profile.cpp

