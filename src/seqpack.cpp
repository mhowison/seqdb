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

#include <string.h>
#include <omp.h>
#include "seqpack.h"

#define PROGNAME "seqpack"
#include "util.h"

using namespace std;

static char bases[5] = { 'N', 'A', 'T', 'C', 'G' };

/* hard-coded phred33 quality scores */
static char quals[51] = { '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S' };

SeqPack::SeqPack(int length)
{
	this->length = length;

	memset(enc_base, 0x00, SEQPACK_ENC_SIZE);
	memset(enc_qual, 0x00, SEQPACK_ENC_SIZE);
	memset(dec_base, 0x00, SEQPACK_DEC_SIZE);
	memset(dec_qual, 0x00, SEQPACK_DEC_SIZE);

	/* create encoding tables */
	for (uint8_t i=0; i<=4; i++) {
		int j = (int)bases[i];
		if (j < 0 || j > 127) ERROR("internal error building encoding table")
		enc_base[j] = i;
	}
	for (uint8_t i=1; i<=51; i++) {
		int j = (int)quals[i-1];
		if (j < 0 || j > 127) ERROR("internal error building encoding table")
		enc_qual[j] = i;
	}

	/* create decoding tables */
	for (int i=0; i<=4; i++) {
		for (int j=1; j<=51; j++) {
			dec_base[i*51 + j] = bases[i];
			dec_qual[i*51 + j] = quals[j-1];
		}
	}
}

void SeqPack::setEncBase(const uint8_t* _enc_base, size_t len)
{
	if (len != SEQPACK_ENC_SIZE) ERROR("incorrect size for encoding table")
	memcpy(enc_base, _enc_base, SEQPACK_ENC_SIZE);
}

void SeqPack::setEncQual(const uint8_t* _enc_qual, size_t len)
{
	if (len != SEQPACK_ENC_SIZE) ERROR("incorrect size for encoding table")
	memcpy(enc_qual, _enc_qual, SEQPACK_ENC_SIZE);
}

void SeqPack::setDecBase(const char* _dec_base, size_t len)
{
	if (len != SEQPACK_DEC_SIZE) ERROR("incorrect size for decoding table")
	memcpy(dec_base, _dec_base, SEQPACK_DEC_SIZE);
}

void SeqPack::setDecQual(const char* _dec_qual, size_t len)
{
	if (len != SEQPACK_DEC_SIZE) ERROR("incorrect size for decoding table")
	memcpy(dec_qual, _dec_qual, SEQPACK_DEC_SIZE);
}

void SeqPack::pack(
		const char* seq,
		const char* qual,
		uint8_t* record)
{
	for (int i=0; i<length; i++) {
		record[i] = enc_base[(int)seq[i]]*51 + enc_qual[(int)qual[i]];
	}
}

void SeqPack::parpack(
		size_t n,
		const char* src,
		uint8_t* dst)
{
	#pragma omp parallel for
	for (size_t i=0; i<n; i++) {
		pack(src + 2*i*length, src + (2*i+1)*length, dst + i*length);
	}
}

void SeqPack::unpack(
		const uint8_t* record,
		char* seq,
		char* qual)
{
	for (int i=0; i<length; i++) {
		uint8_t j = record[i];
		seq[i] = dec_base[j];
		qual[i] = dec_qual[j];
	}
}

void SeqPack::parunpack(
		size_t n,
		const uint8_t* src,
		char* dst)
{
	#pragma omp parallel for
	for (size_t i=0; i<n; i++) {
		unpack(src + i*length, dst + 2*i*length, dst + (2*i+1)*length);
	}
}

void SeqPack::unpack_v2(
		const uint8_t* record,
		char* seq,
		char* qual)
{
	for (int i=0; i<length; i++) {
		uint8_t j = record[i];
		seq[i] = bases[j/51];
		qual[i] = quals[j%51];
	}
}

