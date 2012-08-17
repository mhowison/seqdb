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

#define PROGNAME "seqdb-extract"
#include "util.h"

using namespace std;

void print_usage()
{
	cout << "\n"
"usage: "PROGNAME" SEQDB\n"
"\n"
"Converts the SEQDB input file to FASTQ format, printing to stdout.\n"
"\n"
"Example usage:\n"
PROGNAME" 1.seqdb >1.fastq\n"
	<< endl;
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

	if (optind >= argc)
		ARG_ERROR("you must specify an input file")

	/* Open the input. */
	SeqDB* db = SeqDB::open(argv[optind]);

	ios_base::sync_with_stdio(false);
	setvbuf(stdout, NULL, _IOFBF, 1024*1024);

	db->exportFASTQ(stdout);

	/* Cleanup. */
	delete db;

	return EXIT_SUCCESS;
}

