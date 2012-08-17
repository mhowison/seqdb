/*
 * SeqDB - storage model for Next Generation Sequencing data
 *
 * Copyright 2011-2012, Brown University, Providence, RI. All Rights Reserved.
 *
 * This file is part of SeqDB.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose other than its incorporation into a
 * commercial product is hereby granted without fee, provided that the
 * above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Brown University not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR ANY
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "seqdb.h"
#include "h5seqdb.h"

#define PROGNAME "seqdb"
#include "util.h"

using namespace std;

SeqDB* SeqDB::open(const char* filename)
{
	if (H5Fis_hdf5(filename)) {
		size_t blocksize = 16*1024;
		char* s = getenv("SEQDB_BLOCKSIZE");
		if (s != NULL) {
			blocksize = atoi(s);
			NOTIFY("overriding default HDF5 blocksize to " << blocksize)
		}
		return new H5SeqDB(filename, READ, 0, 0, blocksize);
	} else {
		ERROR("cannot determine storage layer for '" << filename << "'")
	}
}

SeqDB* SeqDB::create(const char* filename, char mode, size_t slen, size_t ilen)
{
	size_t blocksize = 16*1024;
	char* s = getenv("SEQDB_BLOCKSIZE");
	if (s != NULL) {
		blocksize = atoi(s);
		NOTIFY("overriding default HDF5 blocksize to " << blocksize)
	}
	return new H5SeqDB(filename, mode, slen, ilen, blocksize);
}

SeqDB::SeqDB(const char* filename, char mode, size_t slen, size_t ilen)
{
	this->filename = filename;
	this->mode = mode;
	this->slen = slen;
	this->ilen = ilen;

	if ((mode != READ) && (mode != TRUNCATE) && (mode != APPEND)) {
		ERROR("mode must be READ, TRUNCATE, or APPEND")
	}

	qual_offset = 33;
	pack = new SeqPack(slen);
}

SeqDB::~SeqDB()
{
	delete pack;
}

void SeqDB::write(const Sequence&) {}
bool SeqDB::read(Sequence&) {}
void SeqDB::readAt(size_t, Sequence&) {}
void SeqDB::importFASTQ(FASTQ*) {}
void SeqDB::exportFASTQ(FILE*) {}

