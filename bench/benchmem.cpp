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
#include <iostream>
#include <vector>
#include <omp.h>
#include <zlib.h>
#include <numa.h>
#include "fastq.h"
#include "seqdb.h"
#include "blosc.h"
#include "papi-util.h"

#define PROGNAME "benchmem"
#include "util.h"

#if PIN_CORE0
#define _GNU_SOURCE
#include <sched.h>
#endif

#define TIC(label)\
	zero_papi_counters(&counters);\
	start_papi_counters(&counters);

#define TOC(label)\
	stop_papi_counters(&counters);\
	sprintf(prefix, STRINGIFY(label) "\t%zu", nreads);\
	print_papi_counters(prefix, &counters);

#define CHECK(buf)\
	if (memcmp(buffer, buf, LEN * nreads) != 0) ERROR("buffers do not match");

#define ZERO(buf) \
	flush_cache();\
	memset(buf, 0x00, nbytes);

using namespace std;

#define ILEN 56
#define SLEN 100
#define LEN 256
#define ULEN 156

#define BLOCKSIZE (256*1024*1024)
#define FLUSHSIZE (8*1024*1024)

#define BLOSC_LEVEL 4

static int nthreads;

static void flush_cache()
{
#pragma omp parallel
{
	const size_t nbytes = FLUSHSIZE*sizeof(long);
	long* buf = (long*)numa_alloc_local(nbytes);
	if (!buf) ERROR("failed to malloc flush buffer (" << nbytes << " bytes)")

	#pragma omp for
	for (size_t i=0; i<FLUSHSIZE; i++) buf[i] = random();

	numa_free(buf, nbytes);
} // parallel
}

static size_t test_blosc_compress(
		void* buffer1,
		void* buffer2,
		vector<size_t>& index,
		size_t len,
		size_t nreads,
		size_t nbytes,
		int level)
{
	char* src = (char*)buffer2;
	char* dst = (char*)buffer1;
	size_t bytes_left = len * nreads;
	size_t blocksize = BLOCKSIZE;
	size_t dstsize = nbytes;
	while (bytes_left > 0) {
		if (bytes_left < blocksize) blocksize = bytes_left;
		if (dstsize < blocksize + BLOSC_MAX_OVERHEAD)
			ERROR("buffer too small for blosc_compress")
#if DEBUG
		fprintf(stderr, "benchmem: blosc_compress: %zu bytes left\n", bytes_left);
		fprintf(stderr, "benchmem: blosc_compress: %zu byte block\n", blocksize);
#endif
		int cbytes = blosc_compress(level, 0, 1, blocksize, src, dst,
											blocksize + BLOSC_MAX_OVERHEAD);
		if (cbytes < 0) { ERROR("blosc_compress error " << cbytes); }
		else if (cbytes == 0) { ERROR("blosc_compress: unable to compress"); }
#if DEBUG
		fprintf(stderr, "benchmem: blosc_compress: %d / %zu bytes\n", cbytes, blocksize);
#endif
		src += blocksize;
		bytes_left -= blocksize;
		index.push_back(cbytes);
		dst += cbytes;
		dstsize -= cbytes;
	}
	return blocksize;
}

static void test_blosc_decompress(
		void* buffer1,
		void* buffer2,
		vector<size_t>& index,
		size_t last_blocksize)
{
	char* src = (char*)buffer1;
	char* dst = (char*)buffer2;
	size_t blocksize = BLOCKSIZE;
	for (unsigned i=0; i<(index.size()-1); i++) {
		int cbytes = blosc_decompress(src, dst, blocksize);
		if (cbytes <= 0) ERROR("blosc_decompress error");
#if DEBUG
		fprintf(stderr, "benchmem: blosc_decompress: %d / %zu bytes\n", cbytes, blocksize);
#endif
		src += index[i];
		dst += blocksize;
	}
	int cbytes = blosc_decompress(src, dst, last_blocksize);
	if (cbytes <= 0) ERROR("blosc_decompress error");
#if DEBUG
	fprintf(stderr, "benchmem: blosc_decompress: %d / %zu bytes\n", cbytes, last_blocksize);
#endif
}

static size_t test_zlib_compress(
		void* buffer1,
		void* buffer2,
		vector<size_t>& index,
		size_t nreads,
		size_t nbytes,
		int level
		)
{
	Bytef* src = (Bytef*)buffer2;
	Bytef* dst = (Bytef*)buffer1;
	size_t blocksize = BLOCKSIZE;
	size_t bytes_left = LEN * nreads;
	while (bytes_left > 0) {
		if (bytes_left < blocksize) blocksize = bytes_left;
		uLongf cbytes = nbytes;
		int ret = compress2(dst, &cbytes, src, blocksize, level);
		if (ret != Z_OK) ERROR(zError(ret))
		src += blocksize;
		bytes_left -= blocksize;
		index.push_back(cbytes);
		dst += cbytes;
#if DEBUG
		fprintf(stderr, "benchmem: zlib_compress: %lu / %zu bytes\n", cbytes, blocksize);
#endif
	}
	return blocksize;
}

static void test_zlib_decompress(
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
#if DEBUG
		fprintf(stderr, "benchmem: zlib_decompress: %lu bytes\n", ubytes);
#endif
		src += index[i];
		dst += blocksize;
	}
	uLongf ubytes = last_blocksize;
	int ret = uncompress(dst, &ubytes, src, index.back());
	if (ret != Z_OK) ERROR(zError(ret))
#if DEBUG
	fprintf(stderr, "benchmem: zlib_decompress: %lu bytes\n", ubytes);
#endif
}

int main(int argc, char** argv)
{
	/* set thread affinity to core 0
	 * WARNING: OpenMP threads inherit this CPU mask and it
	 * overrides KMP_AFFINITY! */
#if PIN_CORE0
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	CPU_SET(0, &cpu_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
#endif

	if (argc < 2) {
		cerr << "benchmem: please specify an input FASTQ file" << endl;
		exit(0);
	} else if (argc < 3) {
		cerr << "benchmem: please specify number of sequences to read" << endl;
		exit(0);
	}
	const char* filename = argv[1];
	size_t nreads = atol(argv[2]);
	size_t seq_offset = ILEN * nreads;
	cout << "benchmem: preparing for " << nreads << " reads" << endl;
	size_t nblocks = (LEN * nreads - 1) / BLOCKSIZE + 1;
	size_t nbytes = compressBound(LEN * nreads) + nblocks * BLOSC_MAX_OVERHEAD;
	size_t nfound = 0;

	mmapFASTQ fastq(filename);
	SeqPack pack(SLEN);
	Sequence seq;

	void* buffer = numa_alloc_onnode(nbytes, 0);
	if (!buffer) ERROR("failed to malloc buffer (" << nbytes << " bytes)")
	void* buffer1 = numa_alloc_onnode(nbytes, 0);
	if (!buffer1) ERROR("failed to malloc buffer1 (" << nbytes << " bytes)")
	void* buffer2 = numa_alloc_onnode(nbytes, 0);
	if (!buffer2) ERROR("failed to malloc buffer2 (" << nbytes << " bytes)")

	omp_set_dynamic(0);

	cout << "benchmem: found " << omp_get_num_procs() << " procs" << endl;

	nthreads = omp_get_max_threads();
	cout << "benchmem: found " << nthreads << " threads" << endl;

	init_papi_library();
	init_papi_threads();

	CounterGroup counters;
	init_papi_counters(PAPI_UTIL_NEHALEM_L3, &counters);

	char prefix[128];

	/***** Load FASTQ file *****/
	TIC(load_fastq)
	{
		char* ids = (char*)buffer;
		char* seqs = (char*)buffer + seq_offset;
		while (fastq.next(seq)) {
			memcpy(ids, seq.idline.data(), ILEN);
			ids += ILEN;
			memcpy(seqs, seq.seq.data(), SLEN);
			seqs += SLEN;
			memcpy(seqs, seq.qual.data(), SLEN);
			seqs += SLEN;
			if (++nfound == nreads) break;
		}
	}
	TOC(load_fastq)

	NOTIFY("found " << nfound << " reads")

	memcpy(buffer1, buffer, LEN * nreads);

	CHECK(buffer1)
	ZERO(buffer2)

	/***** MEMCPY *****/
	TIC(time_memcpy)
	memcpy(buffer2, buffer1, LEN * nreads);
	TOC(time_memcpy)

	CHECK(buffer2)
	ZERO(buffer1)

	/***** SeqPack *****/

#pragma omp parallel
{
	#pragma omp master
	{	
		TIC(time_pack)
		memcpy(buffer1, buffer2, seq_offset);
	}

	const char* src = (char*)buffer2 + seq_offset;
	uint8_t* dst = (uint8_t*)buffer1 + seq_offset;

	#pragma omp barrier
	#pragma omp for
	for (size_t i=0; i<nreads; i++) {
		pack.pack(src + 2*i*SLEN, src + (2*i+1)*SLEN, dst + i*SLEN);
	}

	#pragma omp barrier
	#pragma omp master
	{	
		TOC(time_pack)
	}
} // parallel

	ZERO(buffer2)

#pragma omp parallel
{
	#pragma omp master
	{
		TIC(time_unpack)
		memcpy(buffer2, buffer1, seq_offset);
	}

	const uint8_t* src = (uint8_t*)buffer1 + seq_offset;
	char* dst = (char*)buffer2 + seq_offset;

	#pragma omp barrier
	#pragma omp for
	for (size_t i=0; i<nreads; i++) {
		pack.unpack(src + i*SLEN, dst + 2*i*SLEN, dst + (2*i+1)*SLEN);
	}

	#pragma omp barrier
	#pragma omp master
	{
		TOC(time_unpack)
	}
} // parallel

	CHECK(buffer2)
	ZERO(buffer1)

	/***** BLOSC *****/
	vector<size_t> block_index;
	size_t last_blocksize;

	block_index.clear();

	blosc_set_nthreads(nthreads);

	TIC(time_blosc_compress)
	last_blocksize = test_blosc_compress(
			buffer1, buffer2, block_index, LEN, nreads, nbytes, BLOSC_LEVEL);
	TOC(time_blosc_compress)

	ZERO(buffer2)

	TIC(time_blosc_decompress)
	test_blosc_decompress(buffer1, buffer2, block_index, last_blocksize);
	TOC(time_blosc_decompress)

	CHECK(buffer2)
	ZERO(buffer1)

	/***** SeqPack + BLOSC *****/
	block_index.clear();

#pragma omp parallel
{
	#pragma omp master
	{	
		TIC(time_pack_blosc)
		memcpy(buffer1, buffer2, seq_offset);
	}

	const char* src = (char*)buffer2 + seq_offset;
	uint8_t* dst = (uint8_t*)buffer1 + seq_offset;

	#pragma omp barrier
	#pragma omp for
	for (size_t i=0; i<nreads; i++) {
		pack.pack(src + 2*i*SLEN, src + (2*i+1)*SLEN, dst + i*SLEN);
	}

	#pragma omp barrier
	#pragma omp master
	{	
		last_blocksize = test_blosc_compress(
			buffer2, buffer1, block_index, ULEN, nreads, nbytes, BLOSC_LEVEL);
		TOC(time_pack_blosc)
	}
} // parallel

	ZERO(buffer1)

#pragma omp parallel
{
	#pragma omp master
	{
		TIC(time_unpack_blosc)
		test_blosc_decompress(buffer2, buffer1, block_index, last_blocksize);
		memcpy(buffer2, buffer1, seq_offset);
	}

	const uint8_t* src = (uint8_t*)buffer1 + seq_offset;
	char* dst = (char*)buffer2 + seq_offset;

	#pragma omp barrier
	#pragma omp for
	for (size_t i=0; i<nreads; i++) {
		pack.unpack(src + i*SLEN, dst + 2*i*SLEN, dst + (2*i+1)*SLEN);
	}

	#pragma omp barrier
	#pragma omp master
	{
		TOC(time_unpack_blosc)
	}
} // parallel

	CHECK(buffer2)
	ZERO(buffer1)

	/***** ZLIB *****/

	/* This test is much slower than the others... skip when the env. var.
	   is set. */
	if (getenv("BENCHMEM_SKIP_ZLIB") == NULL)
	{
		block_index.clear();

		TIC(time_zlib_compress)
		last_blocksize = test_zlib_compress(
							buffer1, buffer2, block_index, nreads, nbytes, 6);
		TOC(time_zlib_compress)

		ZERO(buffer2)

		TIC(time_zlib_decompress)
		test_zlib_decompress(buffer1, buffer2, block_index, last_blocksize);
		TOC(time_zlib_decompress)

		CHECK(buffer2)
	}

	/***** cleanup *****/
	numa_free(buffer, nbytes);
	numa_free(buffer1, nbytes);
	numa_free(buffer2, nbytes);
}

