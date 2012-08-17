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

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <dsrc.h>
#include "fastq.h"
#include "seqdb.h"

#define PROGNAME "benchrand"
#include "util.h"

using namespace std;

static long get_current_usec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (1000000L * (long)tv.tv_sec) + (long)tv.tv_usec;
}

int main(int argc, char** argv)
{
	if (argc < 4) {
		cerr << "usage: benchrand [SeqDB] [DSRC] [N]" << endl;
		exit(0);
	}
	const char* seqdb_file = argv[1];
	const char* dsrc_file = argv[2];
	size_t nreads = atol(argv[3]);

	SeqDB* seqdb = SeqDB::open(seqdb_file);
	Sequence seq;

	/* generate random indices */
	vector<size_t> ids;
	for (size_t i=0; i<seqdb->size(); i++) ids.push_back(i);
	random_shuffle(ids.begin(), ids.end());

	ofstream idsFile("benchrand.ids.txt");
	for (size_t i=0; i<nreads; i++) idsFile << ids[i] << '\n';
	idsFile.close();

	ofstream seqdbTimes("benchrand.seqdb.txt");
	ofstream seqdbFastq("benchrand.seqdb.fq");
	for (size_t i=0; i<nreads; i++) {
		long start = get_current_usec();
		seqdb->readAt(ids[i], seq);
		long end = get_current_usec();
		seqdbTimes << (end - start) << '\n';
		seqdbFastq << seq;
	}
	seqdbTimes.close();
	seqdbFastq.close();

	delete seqdb;

	DsrcFile dsrc;
	FastqRecord record;
	FastqFile dsrcFastq;

	if (!dsrc.StartExtract(dsrc_file)) ERROR("cannot start dsrc extraction")

	ofstream dsrcTimes("benchrand.dsrc.txt");
	dsrcFastq.Create("benchrand.dsrc.fq");
	for (size_t i=0; i<nreads; i++) {
		long start = get_current_usec();
		dsrc.ExtractRecord(record, ids[i]);
		long end = get_current_usec();
		dsrcFastq.WriteRecord(record);
		dsrcTimes << (end - start) << '\n';
	}
	dsrcTimes.close();
	dsrcFastq.Close();

	if (dsrc.IsError()) ERROR("error in dsrc extraction")
	dsrc.FinishExtract();

	exit(EXIT_SUCCESS);
}

