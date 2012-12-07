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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "fastq.h"

#define PROGNAME "fastq"
#include "util.h"

#define FASTQ_DELIM '@'

using namespace std;

FASTQ::FASTQ(const char* filename)
{
	this->filename = filename;
	nline = 0;
	line.reserve(128);
}

void FASTQ::check_delim(char delim)
{
	if (delim != FASTQ_DELIM)
		ERROR("id line for '" << filename << "':" << nline <<
		      " does not start with '" << FASTQ_DELIM << "' (" << delim << ")")
}

void FASTQ::check_plus()
{
	next_line(line);
	if (line.size() == 0 || line[0] != '+')
		ERROR("sequence/quality separator for '" <<
		      filename << "':" << nline << " is not '+'")
}

mmapFASTQ::mmapFASTQ(const char* filename) : FASTQ(filename)
{
	nchar = 0;

	/* get file size */
	struct stat st;
	errno = 0;
	stat(filename, &st);
	if (errno) PERROR("could not stat file '" << filename << "'")
	size = st.st_size;
	NOTIFY("opening file '" << filename << "' with size " << size)

	errno = 0;
	fd = open(filename, O_RDONLY);
	if (errno) PERROR("could not open file '" << filename << "'")

	errno = 0;
	input = (char*)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (errno) PERROR("could not mmap file '" << filename << "'")
}

mmapFASTQ::~mmapFASTQ()
{
	errno = 0;
	munmap(input, size);
	if (errno) PERROR("could not unmmap file '" << filename << "'")

	errno = 0;
	close(fd);
	if (errno) PERROR("could not close file '" << filename << "'")
}

bool mmapFASTQ::good() { return (nchar < size); }

bool mmapFASTQ::next(Sequence& seq)
{
	if (nchar >= size) return false;

	/* ID line. */
	next_line(seq.idline);

	check_delim(seq.idline.at(0));

	/* Sequence line. */
	next_line(seq.seq);

	/* Plus line. */
	check_plus();

	/* Quality line. */
	next_line(seq.qual);

	return (nchar < size);
}

void mmapFASTQ::next_line(string& buf)
{
	buf.clear();
	register char c = '\0';
	while (nchar < size) {
		c = input[nchar++];
		if (c == '\n') break;
		buf.push_back(c);
	}
	nline++;
}

bool mmapFASTQ::next_line(char* buf, size_t count)
{
	//if (nchar + count + 1 > size) return false;
	char* start = input + nchar;
	char* c = (char*)memchr(start, '\n', count + 1);
	if (c == NULL)
		ERROR("line at '" << filename << "':" << nline <<
		      " has more than " << count << " characters")
	size_t n = c - start + 1;
	memcpy(buf, start, n - 1);
	nchar += n;
	nline++;
	return (nchar <= size);
}

streamFASTQ::streamFASTQ(const char* filename) : FASTQ(filename)
{
	if (strcmp(filename, "-") == 0) {
		this->filename = "<stdin>";
		input = &cin;
	} else {
		input_file = new ifstream(filename);
		input = input_file;
	}
}

streamFASTQ::~streamFASTQ()
{
	if (input != &cin) delete input;
}

bool streamFASTQ::good() { return input->good(); }

bool streamFASTQ::next(Sequence& seq)
{
	/* ID line. */
	if (!getline(*input, seq.idline)) return false;
	nline++;

	check_delim(seq.idline.at(0));

	/* Sequence line. */
	if (!getline(*input, seq.seq)) return false;
	nline++;

	/* Plus line. */
	check_plus();

	/* Quality line. */
	if (!getline(*input, seq.qual)) return false;
	nline++;

	return input->good();
}

void streamFASTQ::next_line(string& buf)
{
	getline(*input, buf);
	nline++;
}

bool streamFASTQ::next_line(char* buf, size_t count)
{
	getline(*input, line);
	if (line.size() > count)
		ERROR("line at '" << filename << "':" << nline <<
		      " has more than " << count << " characters")
	memcpy(buf, line.data(), line.size());
	nline++;
	return input->good();
}

