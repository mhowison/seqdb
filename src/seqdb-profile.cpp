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
#include <stdio.h>
#include <iostream>
#include <vector>
#include "seqdb.h"
#include "fastq.h"

#define PROGNAME "seqdb-profile"
#include "util.h"

using namespace std;

void print_usage()
{
	cout <<
"\n"
"usage: "PROGNAME" [FASTQ [FASTQ ...]]\n"
"\n"
"Reads each FASTQ input file and prints a histogram of the lengths of its\n"
"ID lines and sequences. If no inputs are specified, stdin is used.\n"
	<< endl;
}

void profile(FASTQ* input)
{
	vector<size_t> ilen(101, 0);
	vector<size_t> slen(101, 0);
	Sequence seq;

	size_t n = 0;
	while (input->next(seq))
	{
		n++;

		size_t i = seq.idline.size();
		if (i >= ilen.size()) ilen.resize(i+1, 0);
		ilen[i] += 1;

		size_t s = seq.seq.size();
		size_t q = seq.qual.size();
		if (s != q) ERROR("mismatching sequence and quality size at read " << n)
		if (s >= slen.size()) slen.resize(s+1, 0);
		slen[s]++;
	}

	fprintf(stderr, "  total reads: %12lu\n", n);
	cerr << "  ID lengths:" << endl;

	for (size_t i=0; i<ilen.size(); i++)
		if (ilen[i] > 0)
			fprintf(stderr, "         %5lu %12lu\n", i, ilen[i]);
	cerr << "  sequence lengths:" << endl;
	for (size_t i=0; i<slen.size(); i++)
		if (slen[i] > 0)
			fprintf(stderr, "         %5lu %12lu\n", i, slen[i]);
}

int main(int argc, char** argv)
{
	int c;
	while ((c = getopt(argc, argv, "hv")) != -1)
		switch (c) {
			case 'v':
				PRINT_VERSION
				return EXIT_SUCCESS;
			case 'h':
			default:
				print_usage();
				return EXIT_SUCCESS;
		}

	/* Open input files. */
	for (int i=optind; i<argc; i++) {
		NOTIFY("profiling FASTQ records from '" << argv[i] << "'")
		FASTQ* input = new mmapFASTQ(argv[i]);
		profile(input);
		delete input;
	}

	/* Or default to stdin. */
	if (optind == argc) {
		NOTIFY("profiling FASTQ records from '<stdin>'")
		FASTQ* input = new streamFASTQ("-");
		profile(input);
		delete input;
	}

	return EXIT_SUCCESS;
}

