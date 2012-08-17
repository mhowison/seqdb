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
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <omp.h>
#include "fastq.h"
#include "seqdb.h"
#include "blosc.h"
#include "zlib.h"

#if LIKWID
extern "C" {
#include "likwid.h"
}
#endif

#define PROGNAME "benchmem"
#include "util.h"

using namespace std;

#define ILEN 56
#define SLEN 100
#define LEN 256
#define ULEN 156

#define BLOCKSIZE 256*1024*1024

size_t test_blosc_compress(
		void* buffer1,
		void* buffer2,
		vector<size_t>& index,
		size_t len,
		size_t nreads,
		int level)
{
	char* src = (char*)buffer2;
	char* dst = (char*)buffer1;
	size_t blocksize = BLOCKSIZE;
	size_t bytes_left = len * nreads;
	size_t nbytes, cbytes, nblock;
	while (bytes_left > 0) {
		if (bytes_left < blocksize) blocksize = bytes_left;
		blosc_compress(level, 0, 1, blocksize,
				src, dst, blocksize + BLOSC_MAX_OVERHEAD); 
		blosc_cbuffer_sizes(dst, &nbytes, &cbytes, &nblock);
		src += blocksize;
		bytes_left -= blocksize;
		index.push_back(cbytes);
		dst += cbytes;
	}
	return blocksize;
}

void test_blosc_decompress(
		void* buffer1,
		void* buffer2,
		vector<size_t>& index,
		size_t last_blocksize)
{
	char* src = (char*)buffer1;
	char* dst = (char*)buffer2;
	size_t blocksize = BLOCKSIZE;
	for (unsigned i=0; i<(index.size()-1); i++) {
		blosc_decompress(src, dst, blocksize);
		src += index[i];
		dst += blocksize;
	}
	blosc_decompress(src, dst, last_blocksize);
}

size_t test_zlib_compress(
		void* buffer1,
		void* buffer2,
		vector<size_t>& index,
		size_t nreads,
		int level
		)
{
	Bytef* src = (Bytef*)buffer2;
	Bytef* dst = (Bytef*)buffer1;
	size_t blocksize = BLOCKSIZE;
	size_t bytes_left = LEN * nreads;
	while (bytes_left > 0) {
		if (bytes_left < blocksize) blocksize = bytes_left;
		uLongf cbytes = compressBound(blocksize);
		int ret = compress2(dst, &cbytes, src, blocksize, level);
		if (ret != Z_OK) ERROR(zError(ret))
		src += blocksize;
		bytes_left -= blocksize;
		index.push_back(cbytes);
		dst += cbytes;
	}
	return blocksize;
}

void test_zlib_decompress(
		void* buffer1,
		void* buffer2,
		vector<size_t>& index,
		size_t last_blocksize)
{
	Bytef* src = (Bytef*)buffer1;
	Bytef* dst = (Bytef*)buffer2;
	size_t blocksize = BLOCKSIZE;
	for (unsigned i=0; i<(index.size()-1); i++) {
		uLongf ubytes = blocksize;
		int ret = uncompress(dst, &ubytes, src, index[i]);
		if (ret != Z_OK) ERROR(zError(ret))
		src += index[i];
		dst += blocksize;
	}
	uLongf ubytes = last_blocksize;
	int ret = uncompress(dst, &ubytes, src, index.back());
	if (ret != Z_OK) ERROR(zError(ret))
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		cerr << "benchmem: please specify an input FASTQ file" << endl;
		exit(0);
	} else if (argc < 3) {
		cerr << "benchmem: please specify number of sequences to read" << endl;
		exit(0);
	}
	const char* filename = argv[1];
	size_t nreads = atol(argv[2]);
	size_t nbytes = compressBound(LEN * nreads);
	size_t nfound = 0;

#if LIKWID
	likwid_markerInit();
#endif

	FASTQ fastq(filename);
	SeqPack pack(SLEN);
	Sequence seq;

	void* buffer1 = malloc(nbytes);
	if (!buffer1) ERROR("failed to malloc buffer1 (" << nbytes << " bytes)")
	void* buffer2 = malloc(nbytes);
	if (!buffer2) ERROR("failed to malloc buffer2 (" << nbytes << " bytes)")

	TIC(load_fastq)
	{
		char* dst = (char*)buffer1;
		while (fastq.next(seq)) {
			strncpy(dst, seq.idline.data(), ILEN);
			dst += ILEN;
			memcpy(dst, seq.seq.data(), SLEN);
			dst += SLEN;
			memcpy(dst, seq.qual.data(), SLEN);
			dst += SLEN;
			if (++nfound == nreads) break;
		}
	}
	TOC(load_fastq)

	NOTIFY("found " << nfound << " reads")

	/***** MEMCPY *****/
	TIC(time_memcpy)
	memcpy(buffer2, buffer1, LEN * nreads);
	TOC(time_memcpy)

	memset(buffer1, 0x00, nbytes);

	/***** SEQDB *****/
	TIC(time_pack)
#pragma omp parallel for
	for (size_t i=0; i<nreads; i++) {
		char* src = (char*)buffer2 + LEN * i;
		uint8_t* dst = (uint8_t*)buffer1 + ULEN * i;
		memcpy(dst, src, ILEN);
		pack.pack(src + ILEN, src + ULEN, dst + ILEN);
	}
	TOC(time_pack)

	memset(buffer2, 0x00, nbytes);

	TIC(time_unpack)
#pragma omp parallel for
	for (size_t i=0; i<nreads; i++) {
		uint8_t* src = (uint8_t*)buffer1 + ULEN * i;
		char* dst = (char*)buffer2 + LEN * i;
		memcpy(dst, src, ILEN);
		pack.unpack(src + ILEN, dst + ILEN, dst + ULEN);
	}
	TOC(time_unpack)

	memset(buffer2, 0x00, nbytes);

	TIC(time_unpack_v2)
#pragma omp parallel for
	for (size_t i=0; i<nreads; i++) {
		uint8_t* src = (uint8_t*)buffer1 + ULEN * i;
		char* dst = (char*)buffer2 + LEN * i;
		memcpy(dst, src, ILEN);
		pack.unpack_v2(src + ILEN, dst + ILEN, dst + ULEN);
	}
	TOC(time_unpack_v2)

	memset(buffer1, 0x00, nbytes);

	/***** BLOSC *****/
	int bt = omp_get_max_threads();
	cerr << "benchmem: setting BLOSC nthreads = " << bt << endl;
	blosc_set_nthreads(bt);

	vector<size_t> block_index;
	size_t last_blocksize;

#if 0
	/* level 2 */
	TIC(time_blosc_compress2)
	last_blocksize = test_blosc_compress(
								buffer1, buffer2, block_index, LEN, nreads, 2);
	TOC(time_blosc_compress2)

	memset(buffer2, 0x00, nbytes);

	TIC(time_blosc_decompress2)
	test_blosc_decompress(buffer1, buffer2, block_index, last_blocksize);
	TOC(time_blosc_decompress2)

	memset(buffer1, 0x00, nbytes);
#endif

	/* level 4 */
	block_index.clear();

	TIC(time_blosc_compress4)
	last_blocksize = test_blosc_compress(
								buffer1, buffer2, block_index, LEN, nreads, 4);
	TOC(time_blosc_compress4)

	memset(buffer2, 0x00, nbytes);

	TIC(time_blosc_decompress4)
	test_blosc_decompress(buffer1, buffer2, block_index, last_blocksize);
	TOC(time_blosc_decompress4)

	memset(buffer1, 0x00, nbytes);

#if 0
	/* level 9 */
	block_index.clear();

	TIC(time_blosc_compress9)
	last_blocksize = test_blosc_compress(
								buffer1, buffer2, block_index, LEN, nreads, 9);
	TOC(time_blosc_compress9)

	memset(buffer2, 0x00, nbytes);

	TIC(time_blosc_decompress9)
	test_blosc_decompress(buffer1, buffer2, block_index, last_blocksize);
	TOC(time_blosc_decompress9)

	memset(buffer1, 0x00, nbytes);
#endif

	/***** SeqPack + BLOSC *****/
	block_index.clear();

	TIC(time_pack_blosc)
#pragma omp parallel for
	for (size_t i=0; i<nreads; i++) {
		char* src = (char*)buffer2 + LEN * i;
		uint8_t* dst = (uint8_t*)buffer1 + ULEN * i;
		memcpy(dst, src, ILEN);
		pack.pack(src + ILEN, src + ULEN, dst + ILEN);
	}
	last_blocksize = test_blosc_compress(
								buffer2, buffer1, block_index, ULEN, nreads, 4);
	TOC(time_pack_blosc)

	memset(buffer1, 0x00, nbytes);

	TIC(time_unpack_blosc)
	test_blosc_decompress(buffer2, buffer1, block_index, last_blocksize);
#pragma omp parallel for
	for (size_t i=0; i<nreads; i++) {
		uint8_t* src = (uint8_t*)buffer1 + ULEN * i;
		char* dst = (char*)buffer2 + LEN * i;
		memcpy(dst, src, ILEN);
		pack.unpack(src + ILEN, dst + ILEN, dst + ULEN);
	}
	TOC(time_unpack_blosc)

	memset(buffer1, 0x00, nbytes);

	/***** ZLIB *****/
#if 0
	block_index.clear();

	/* level 2 */
	TIC(time_zlib_compress2)
	last_blocksize = test_zlib_compress(
									buffer1, buffer2, block_index, nreads, 2);
	TOC(time_zlib_compress2)

	memset(buffer2, 0x00, nbytes);

	TIC(time_zlib_decompress2)
	test_zlib_decompress(buffer1, buffer2, block_index, last_blocksize);
	TOC(time_zlib_decompress2)

	memset(buffer1, 0x00, nbytes);
#endif

	/* level 6 */
	block_index.clear();

	TIC(time_zlib_compress6)
	last_blocksize = test_zlib_compress(
									buffer1, buffer2, block_index, nreads, 6);
	TOC(time_zlib_compress6)

	memset(buffer2, 0x00, nbytes);

	TIC(time_zlib_decompress6)
	test_zlib_decompress(buffer1, buffer2, block_index, last_blocksize);
	TOC(time_zlib_decompress6)

#if 0
	memset(buffer1, 0x00, nbytes);

	/* level 9 */
	block_index.clear();

	TIC(time_zlib_compress9)
	last_blocksize = test_zlib_compress(
									buffer1, buffer2, block_index, nreads, 9);
	TOC(time_zlib_compress9)

	memset(buffer2, 0x00, nbytes);

	TIC(time_zlib_decompress9)
	test_zlib_decompress(buffer1, buffer2, block_index, last_blocksize);
	TOC(time_zlib_decompress9)
#endif

#if LIKWID
	likwid_markerClose();
#endif
}

