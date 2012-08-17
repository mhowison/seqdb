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

#include <stdio.h>
#include <string.h>
#include "seq.h"

using namespace std;

void Sequence::clear()
{
	idline.clear();
	seq.clear();
	qual.clear();
}

void Sequence::printFASTQ(
		FILE* f,
		const char* idline,
		const char* seq,
		const char* qual,
		size_t ilen,
		size_t slen)
{
	fwrite(idline, 1, strnlen(idline, ilen), f);
	putc('\n', f);
	fwrite(seq, 1, strnlen(seq, slen), f);
	putc('\n', f);
	putc('+', f);
	putc('\n', f);
	fwrite(qual, 1, strnlen(qual, slen), f);
	putc('\n', f);
}

void Sequence::printFASTQ(
		FILE* f,
		string& idline,
		string& seq,
		string& qual)
{
	fwrite(idline.data(), 1, idline.size(), f);
	putc('\n', f);
	fwrite(seq.data(), 1, seq.size(), f);
	putc('\n', f);
	putc('+', f);
	putc('\n', f);
	fwrite(qual.data(), 1, qual.size(), f);
	putc('\n', f);
}

void Sequence::print() { printFASTQ(stdout, idline, seq, qual); }

ostream& operator << (ostream& o, const Sequence& s)
{
	return o << s.idline << '\n'
	         << s.seq    << '\n'
	         << '+'      << '\n'
	         << s.qual   << '\n';
}

