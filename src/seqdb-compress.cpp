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
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "seqdb.h"
#include "fastq.h"

#define PROGNAME "seqdb-compress"
#include "util.h"

using namespace std;

void print_usage()
{
	cout << "\n"
"usage: "PROGNAME" [-a] SLEN ILEN FASTQ SEQDB\n"
"\n"
"Writes the FASTQ input file into a SEQDB file. Specify '-' for the FASTQ\n"
"file to read from stdin instead.\n"
"\n"
"You must specify a maximum sequence/quality score length (SLEN) and a\n"
"maximum ID length (ILEN).\n"
"\n"
"  -a  append to the SEQDB file instead of truncating\n"
	<< endl;
}

int main(int argc, char** argv)
{
	char mode = SeqDB::TRUNCATE;
	size_t slen = 0;
	size_t ilen = 0;

	int c;
	while ((c = getopt(argc, argv, "hva")) != -1)
		switch (c) {
			case 'a':
				mode = SeqDB::APPEND;
				break;
			case 'v':
				PRINT_VERSION
				return EXIT_SUCCESS;
			case 'h':
			default:
				print_usage();
				return EXIT_SUCCESS;
		}

	if (argc - optind < 4)
		ARG_ERROR("not enough parameters")

	slen = atoi(argv[optind]);
	ilen = atoi(argv[optind+1]);

	if (slen <= 0)
		ARG_ERROR("invalid sequence length (SLEN) " << slen)

	if (ilen <= 0)
		ARG_ERROR("invalid ID length (ILEN) " << ilen)

	/* Open input file. */
	const char* input_name = argv[optind+2];
	FASTQ* input;
	if (strcmp(input_name, "-") == 0) {
		NOTIFY("adding FASTQ records from '<stdin>'")
		input = new streamFASTQ("-");
	} else {
		NOTIFY("adding FASTQ records from '" << input_name << "'")
		input = new mmapFASTQ(input_name);
	}

	SeqDB* db = SeqDB::create(argv[optind+3], mode, slen, ilen);
	size_t nrecords = db->size();
	db->importFASTQ(input);
	NOTIFY("added " << (db->size() - nrecords) << " records")

	/* Cleanup. */
	delete input;
	delete db;

	return EXIT_SUCCESS;
}

