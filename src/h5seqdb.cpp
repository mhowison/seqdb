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
#include <algorithm>
#include <omp.h>
#include "blosc.h"
#include "blosc_filter.h"
#include "h5seqdb.h"

#define PROGNAME "seqdb"
#include "util.h"

#ifndef H5SEQDB_MALLOC_ALIGN
#define H5SEQDB_MALLOC_ALIGN 64
#endif

#ifndef H5SEQDB_MALLOC_PAD
#define H5SEQDB_MALLOC_PAD H5SEQDB_MALLOC_ALIGN
#endif

#define H5SEQDB_SEQ_DATASET "seq"
#define H5SEQDB_ID_DATASET "id"
#define H5SEQDB_GROUP "/seqdb"

#define H5SEQDB_ENC_BASE_ATTR "__encode_base__"
#define H5SEQDB_ENC_QUAL_ATTR "__encode_qualilty__"
#define H5SEQDB_DEC_BASE_ATTR "__decode_base__"
#define H5SEQDB_DEC_QUAL_ATTR "__decode_qualilty__"

#define H5CHK(status) if (status < 0)\
   	ERROR("HDF5 error at " << __FILE__ << ":" << __LINE__)
#define H5TRY(call) do{\
	herr_t status = (herr_t)(call);\
	H5CHK(status)\
}while(0);
#define H5LOC(file,path) "'" << file << ":" << path << "'"

using namespace std;

herr_t hdf5_exit_on_error (hid_t estack_id, void *unused)
{
	H5Eprint(estack_id, stderr);
	exit(EXIT_FAILURE);
}

void H5SeqDB::open_file(const char* filename, hid_t mode)
{
	/* Property lists. */
	hid_t fapl = H5P_DEFAULT;
	//hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
	//H5CHK(fapl)
	//H5Pset_alignment(fapl, blocksize, 0);
	//H5Pset_meta_block_size(fapl, blocksize);

	/* Open. */
	h5file = H5Fopen(filename, mode, fapl);
	if (h5file < 0) ERROR("cannot open file '" << filename << "'")

	/* Cleanup. */
	H5TRY(H5Pclose(fapl))
}

void H5SeqDB::open_datasets(const char* path)
{
	/* Property lists. */
	hid_t dapl = H5P_DEFAULT;
	//hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
	//H5CHK(dapl)
	//H5Pset_chunk_cache(dapl, );

	/* Open. */
	hid_t group = H5Gopen(h5file, path, H5P_DEFAULT);
	if (group < 0) ERROR("cannot open group at " << H5LOC(filename, path))

	sdset = H5Dopen(group, H5SEQDB_SEQ_DATASET, dapl);
	if (sdset < 0) ERROR("cannot open sequence dataset at " << H5LOC(filename, path))

	idset = H5Dopen(group, H5SEQDB_ID_DATASET, dapl);
	if (idset < 0) ERROR("cannot open ID dataset at " << H5LOC(filename, path))

	/* Get data space. */
	sspace = H5Dget_space(sdset);
	H5CHK(sspace)
	ispace = H5Dget_space(idset);
	H5CHK(ispace)

	/* Get dimensions. */
	hsize_t sdims[2] = { 0, 0 };
	hsize_t idims[2] = { 0, 0 };
	H5TRY(H5Sget_simple_extent_dims(sspace, sdims, NULL))
	H5TRY(H5Sget_simple_extent_dims(ispace, idims, NULL))
	if (sdims[0] != idims[0])
		ERROR("sequence and ID datasets at " << H5LOC(filename, path) <<
									"have different sizes: corrupted file?")
	nrecords = sdims[0];
	NOTIFY("found " << nrecords << " records at " << H5LOC(filename, path))
	slen = sdims[1];
	ilen = idims[1];
	NOTIFY("sequence length is " << slen << ", ID length is " << ilen)

	/* Cleanup. */
	//H5TRY(H5Pclose(dapl))
	H5TRY(H5Gclose(group))
}

void H5SeqDB::create_file(const char* filename, hid_t mode)
{
	/* Property lists. */
	hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
	H5CHK(fapl)
	//H5Pset_alignment(fapl, blocksize, 0);
	//H5Pset_meta_block_size(fapl, blocksize);

	/* Create. */
	h5file = H5Fcreate(filename, mode, H5P_DEFAULT, fapl);
	if (h5file < 0) PERROR("cannot create file '" << filename << "'")

	/* Cleanup. */
	H5TRY(H5Pclose(fapl))
}

void H5SeqDB::create_datasets(const char* path)
{
	/* Property lists. */
	hid_t sdcpl = H5Pcreate(H5P_DATASET_CREATE);
	H5CHK(sdcpl)
	hid_t idcpl = H5Pcreate(H5P_DATASET_CREATE);
	H5CHK(idcpl)
	H5TRY(H5Pset_layout(sdcpl, H5D_CHUNKED))
	H5TRY(H5Pset_layout(idcpl, H5D_CHUNKED))
	hsize_t sdims[2] = { blocksize, slen };
	hsize_t idims[2] = { blocksize, ilen };
	H5TRY(H5Pset_chunk(idcpl, 2, idims))
	H5TRY(H5Pset_chunk(sdcpl, 2, sdims))

	/* BLOSC compression. */
	unsigned cd[6];
	cd[4] = 4;
	cd[5] = 0;
	H5TRY(H5Pset_filter(idcpl, FILTER_BLOSC, H5Z_FLAG_OPTIONAL, 6, cd))
	H5TRY(H5Pset_filter(sdcpl, FILTER_BLOSC, H5Z_FLAG_OPTIONAL, 6, cd))

	/* Dimensions. */
	sdims[0] = 1;
	idims[0] = 1;
	hsize_t smaxdims[2] = { H5S_UNLIMITED, slen };
	hsize_t imaxdims[2] = { H5S_UNLIMITED, ilen };
	sspace = H5Screate_simple(2, sdims, smaxdims);
	H5CHK(sspace)
	ispace = H5Screate_simple(2, idims, imaxdims);
	H5CHK(ispace)

	/* Create. */
	hid_t group = H5Gcreate(h5file, path, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	H5CHK(group)
	sdset = H5Dcreate(group,
				H5SEQDB_SEQ_DATASET,
				H5T_NATIVE_UINT8,
				sspace, H5P_DEFAULT, sdcpl, H5P_DEFAULT);
	H5CHK(sdset)
	idset = H5Dcreate(group,
				H5SEQDB_ID_DATASET,
				H5T_NATIVE_CHAR,
				ispace, H5P_DEFAULT, idcpl, H5P_DEFAULT);
	H5CHK(idset)

	/* Store SeqPack tables as attributes. */
	write_array_attribute(H5SEQDB_ENC_BASE_ATTR,
			H5T_NATIVE_UINT8, SEQPACK_ENC_SIZE, pack->getEncBase());
	write_array_attribute(H5SEQDB_ENC_QUAL_ATTR,
			H5T_NATIVE_UINT8, SEQPACK_ENC_SIZE, pack->getEncQual());
	write_array_attribute(H5SEQDB_DEC_BASE_ATTR,
			H5T_NATIVE_CHAR, SEQPACK_DEC_SIZE, pack->getDecBase());
	write_array_attribute(H5SEQDB_DEC_QUAL_ATTR,
			H5T_NATIVE_CHAR, SEQPACK_DEC_SIZE, pack->getDecQual());

	/* Cleanup. */
	H5TRY(H5Pclose(sdcpl))
	H5TRY(H5Pclose(idcpl))
	H5TRY(H5Gclose(group))
}

void H5SeqDB::write_array_attribute(
		const char* name,
		hid_t type,
		hsize_t n,
		const void* array)
{
	hid_t space;
	hid_t attr;

	space = H5Screate_simple(1, &n, NULL);
	H5CHK(space);

	attr = H5Acreate(h5file, name, type, space, H5P_DEFAULT, H5P_DEFAULT);
	H5CHK(attr)

	H5TRY(H5Awrite(attr, type, array))

	/* Cleanup. */
	H5TRY(H5Aclose(attr))
	H5TRY(H5Sclose(space))
}

void H5SeqDB::flush_writes(hsize_t remainder)
{
#ifdef DEBUG
	NOTIFY("flushing " << remainder << " writes at offset " << nrecords)
#endif

	/* Property lists. */
	hid_t dxpl = H5P_DEFAULT;

	/* Extend. */
	hsize_t scount[2] = { nrecords, slen };
	hsize_t icount[2] = { nrecords, ilen };
	H5TRY(H5Dextend(sdset, scount))
	H5TRY(H5Dextend(idset, icount))

	/* Dataspace. */
	sspace = H5Dget_space(sdset);
	H5CHK(sspace)
	ispace = H5Dget_space(idset);
	H5CHK(ispace)

	/* Select. */
	hsize_t offset[2] = { nrecords - remainder, 0 };
	scount[0] = remainder;
	icount[0] = remainder;
	H5TRY(H5Sselect_hyperslab(sspace, H5S_SELECT_SET, offset, NULL, scount, NULL))
	H5TRY(H5Sselect_hyperslab(ispace, H5S_SELECT_SET, offset, NULL, icount, NULL))
	offset[0] = 0;
	H5TRY(H5Sselect_hyperslab(smemspace, H5S_SELECT_SET, offset, NULL, scount, NULL))
	H5TRY(H5Sselect_hyperslab(imemspace, H5S_SELECT_SET, offset, NULL, icount, NULL))

	/* Pack. */
	pack->parpack(remainder, sbuf, pbuf);

	/* Write. */
	H5TRY(H5Dwrite(sdset, H5T_NATIVE_UINT8, smemspace, sspace, dxpl, pbuf))
	H5TRY(H5Dwrite(idset, H5T_NATIVE_CHAR, imemspace, ispace, dxpl, ibuf))
	clear_buffers();

	/* Cleanup. */
	H5TRY(H5Sclose(sspace))
	H5TRY(H5Sclose(ispace))
}

void H5SeqDB::flush_reads()
{
	hsize_t remainder = blocksize;
	if ((nrecords - nread) < blocksize) {
		remainder = nrecords - nread;
	}

#ifdef DEBUG
	NOTIFY("flushing " << remainder << " reads at offset " << nread)
#endif

	/* Property lists. */
	hid_t dxpl = H5P_DEFAULT;

	/* Select. */
	hsize_t offset[2] = { nread, 0 };
	hsize_t scount[2] = { remainder, slen };
	hsize_t icount[2] = { remainder, ilen };
	H5TRY(H5Sselect_hyperslab(sspace, H5S_SELECT_SET, offset, NULL, scount, NULL))
	H5TRY(H5Sselect_hyperslab(ispace, H5S_SELECT_SET, offset, NULL, icount, NULL))
	offset[0] = 0;
	H5TRY(H5Sselect_hyperslab(smemspace, H5S_SELECT_SET, offset, NULL, scount, NULL))
	H5TRY(H5Sselect_hyperslab(imemspace, H5S_SELECT_SET, offset, NULL, icount, NULL))

	/* Read. */
	clear_buffers();
	H5TRY(H5Dread(sdset, H5T_NATIVE_UINT8, smemspace, sspace, dxpl, pbuf))
	H5TRY(H5Dread(idset, H5T_NATIVE_CHAR, imemspace, ispace, dxpl, ibuf))

	/* Unpack. */
	pack->parunpack(remainder, pbuf, sbuf);
}

void H5SeqDB::read_at(size_t i)
{
	/* Property lists. */
	hid_t dxpl = H5P_DEFAULT;

	/* Select. */
	hsize_t offset[2] = { i, 0 };
	hsize_t scount[2] = { 1, slen };
	hsize_t icount[2] = { 1, ilen };
	H5TRY(H5Sselect_hyperslab(sspace, H5S_SELECT_SET, offset, NULL, scount, NULL))
	H5TRY(H5Sselect_hyperslab(ispace, H5S_SELECT_SET, offset, NULL, icount, NULL))
	offset[0] = 0;
	H5TRY(H5Sselect_hyperslab(smemspace, H5S_SELECT_SET, offset, NULL, scount, NULL))
	H5TRY(H5Sselect_hyperslab(imemspace, H5S_SELECT_SET, offset, NULL, icount, NULL))

	/* Read. */
	clear_buffers_at();
	H5TRY(H5Dread(sdset, H5T_NATIVE_UINT8, smemspace, sspace, dxpl, pbuf_at))
	H5TRY(H5Dread(idset, H5T_NATIVE_CHAR, imemspace, ispace, dxpl, ibuf_at))

	/* Unpack. */
	pack->unpack(pbuf_at, sbuf_at, sbuf_at + slen);
}

void H5SeqDB::clear_buffers()
{
	memset(pbuf, 0x00, pbufsize);
	memset(sbuf, 0x00, sbufsize);
	memset(ibuf, 0x00, ibufsize);
}

void H5SeqDB::clear_buffers_at()
{
	memset(pbuf_at, 0x00, slen);
	memset(sbuf_at, 0x00, 2 * slen);;
	memset(ibuf_at, 0x00, ilen);
}

void H5SeqDB::alloc()
{
	pbufsize = slen * blocksize;
	CHECK_ERR(
		posix_memalign((void**)&pbuf,
			H5SEQDB_MALLOC_ALIGN, pbufsize + H5SEQDB_MALLOC_PAD))

	sbufsize = 2 * slen * blocksize;
	CHECK_ERR(
		posix_memalign((void**)&sbuf,
			H5SEQDB_MALLOC_ALIGN, sbufsize + H5SEQDB_MALLOC_PAD))

	ibufsize = ilen * blocksize;
	CHECK_ERR(
		posix_memalign((void**)&ibuf,
			H5SEQDB_MALLOC_ALIGN, ibufsize + H5SEQDB_MALLOC_PAD))

	CHECK_ERR(
		posix_memalign((void**)&pbuf_at,
			H5SEQDB_MALLOC_ALIGN, slen + H5SEQDB_MALLOC_PAD))

	CHECK_ERR(
		posix_memalign((void**)&sbuf_at,
			H5SEQDB_MALLOC_ALIGN, 2*slen + H5SEQDB_MALLOC_PAD))

	CHECK_ERR(
		posix_memalign((void**)&ibuf_at,
			H5SEQDB_MALLOC_ALIGN, ilen + H5SEQDB_MALLOC_PAD))
}

/* public functions */

H5SeqDB::H5SeqDB(
		const char* filename,
		char mode,
		size_t slen,
		size_t ilen,
		size_t blocksize)
	: blocksize(blocksize), SeqDB(filename, mode, slen, ilen)
{
	//H5Eset_auto(H5E_DEFAULT, hdf5_exit_on_error, NULL);
	char* version;
	char* date;
	register_blosc(&version, &date);

	int threads = omp_get_max_threads();
	blosc_set_nthreads(threads);

	NOTIFY("using BLOSC " << version << " (" << date <<
	       ") with " << threads << " thread(s)")

	if (mode == READ) {
		open_file(filename, H5F_ACC_RDONLY);
		/* Opening the datasets will update slen and ilen. */
		open_datasets(H5SEQDB_GROUP);
		nread = 0;
		pack->setLength(this->slen);
	} else {
		if (mode == TRUNCATE) {
			create_file(filename, H5F_ACC_TRUNC);
		} else {
			open_file(filename, H5F_ACC_RDWR);
		}
		create_datasets(H5SEQDB_GROUP);
		nrecords = 0;
	}

	if (this->slen <= 0 || this->ilen <= 0)
		ERROR("sequence or ID length is non-positive")

	alloc();

	hsize_t sdims[2] = { blocksize, this->slen };
	smemspace = H5Screate_simple(2, sdims, NULL);
	H5CHK(smemspace)

	hsize_t idims[2] = { blocksize, this->ilen };
	imemspace = H5Screate_simple(2, idims, NULL);
	H5CHK(imemspace)
}

H5SeqDB::~H5SeqDB()
{
	if (mode == READ) {
		H5TRY(H5Sclose(sspace))
		H5TRY(H5Sclose(ispace))
	} else {
		/* Flush any remaining records. */
		hsize_t remainder = nrecords % blocksize;
		if (remainder) flush_writes(remainder);
	}
	free(pbuf);
	free(sbuf);
	free(ibuf);
	free(pbuf_at);
	free(sbuf_at);
	free(ibuf_at);
	H5TRY(H5Sclose(smemspace))
	H5TRY(H5Sclose(imemspace))
	H5TRY(H5Dclose(sdset))
	H5TRY(H5Dclose(idset))
	H5TRY(H5Fclose(h5file))
}

void H5SeqDB::write(const Sequence& seq)
{
	if (ilen < seq.idline.size())
		ERROR("seq.idline is too large (" << seq.idline.size() <<
		      ") for write (" << ilen << ")")
	if (slen < seq.seq.size())
		ERROR("seq.seq is too large (" << seq.seq.size() <<
		      ") for write (" << slen << ")")
	if (slen < seq.qual.size())
		ERROR("seq.idline is too large (" << seq.qual.size() <<
		      ") for write (" << slen << ")")

	hsize_t remainder = nrecords % blocksize;

	/* Copy idline. */
	char* i = ibuf + ilen * remainder;
	memcpy(i, seq.idline.data(), min(ilen, seq.idline.size()));

	/* Copy seq/qual. */
	char* s = sbuf + 2 * slen * remainder;
	memcpy(s, seq.seq.data(), min(slen, seq.seq.size()));
	memcpy(s + slen, seq.qual.data(), min(slen, seq.qual.size()));

	/* Advance the record counter, and flush if the write bufer is full. */
	nrecords++;
	if (nrecords % blocksize == 0) flush_writes(blocksize);
}

bool H5SeqDB::read(Sequence& seq)
{
	hsize_t remainder = nread % blocksize;

	/* Update buffer if we're back to the first record. */
	if (remainder == 0) flush_reads();

	const char* i = ibuf + ilen * remainder;
	seq.idline.assign(i, strnlen(i, ilen));

	const char* s = sbuf + 2 * slen * remainder;
	seq.seq.assign(s, strnlen(s, slen));
	seq.qual.assign(s + slen, strnlen(s + slen, slen));

	/* Advance the read counter. */
	nread++;

	if (nread <= nrecords) return true;
	else return false;
}

void H5SeqDB::readAt(size_t i, Sequence& seq)
{
	if (i >= nrecords)
		ERROR("can't read record " << i << ": only " << nrecords <<
		      " records exist")

	read_at(i);

	seq.idline.assign(ibuf_at, strnlen(ibuf_at, ilen));
	seq.seq.assign(sbuf_at, strnlen(sbuf_at, slen));
	seq.qual.assign(sbuf_at + slen, strnlen(sbuf_at + slen, slen));
}

void H5SeqDB::importFASTQ(FASTQ* f)
{
	while (f->good())
	{
		hsize_t remainder = nrecords % blocksize;

		/* Read idline. */
		char* i = ibuf + ilen * remainder;
		if (!f->next_line(i, ilen)) break;

		f->check_delim(i[0]);

		/* Read sequence. */
		char* s = sbuf + 2 * slen * remainder;
		if (!f->next_line(s, slen)) break;

		f->check_plus();

		/* Read quality. */
		if (!f->next_line(s + slen, slen)) break;

		/* Advance the record counter, and flush if the write bufer is full. */
		nrecords++;
		if (nrecords % blocksize == 0) flush_writes(blocksize);
	}
}

void H5SeqDB::export_block(FILE* f, hsize_t count)
{
	/* Load the next block. */
	flush_reads();
	const char* i = ibuf;
	const char* s = sbuf;
	/* Print each sequence. */
	for (hsize_t j=0; j<count; j++) {
		Sequence::printFASTQ(f, i, s, s + slen, ilen, slen);
		/* Advance the buffers. */
		i += ilen;
		s += 2*slen;
	}
	nread += count;
}

void H5SeqDB::exportFASTQ(FILE* f)
{
	/* Iterate over the full read blocks. */
	hsize_t nblocks = nrecords / blocksize;
	for (hsize_t i=0; i<nblocks; i++) export_block(f, blocksize);

	/* Export any remaining records. */
	hsize_t remainder = nrecords % blocksize;
	if (remainder) export_block(f, remainder);
}

