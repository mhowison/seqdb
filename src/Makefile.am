# library

lib_LTLIBRARIES = libseqdb.la

libseqdb_la_SOURCES = \
	seq.cpp seqpack.cpp seqdb.cpp h5seqdb.cpp fastq.cpp \
	blosc_filter.c blosc.c blosclz.c shuffle.c \
	seq.h seqpack.h seqdb.h h5seqdb.h fastq.h util.h \
	blosc_filter.h blosc.h blosclz.h shuffle.h

# programs

bin_PROGRAMS = seqdb-compress seqdb-extract seqdb-profile seqdb-mount

LDADD = libseqdb.la

seqdb_compress_SOURCES = seqdb-compress.cpp
seqdb_extract_SOURCES = seqdb-extract.cpp
seqdb_profile_SOURCES = seqdb-profile.cpp
seqdb_mount_SOURCES = seqdb-mount.cpp

# includes

library_includedir = $(includedir)/seqdb
library_include_HEADERS = \
	seq.h seqpack.h seqdb.h h5seqdb.h fastq.h seqdb-version.h

# pkgconfig

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = seqdb.pc
