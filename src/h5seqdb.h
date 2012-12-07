#ifndef __H5SEQDB_H__
#define __H5SEQDB_H__

#include <hdf5.h>
#include "seqdb.h"

class H5SeqDB : public SeqDB {

	public:
		/* functions */
		H5SeqDB(const char* filename, char mode, size_t slen, size_t ilen, size_t blocksize);
		~H5SeqDB();

		/* overrides */
		void write(const Sequence& seq);
		bool read(Sequence& seq);
		void readAt(size_t i, Sequence& seq);
		void importFASTQ(FASTQ* f);
		void exportFASTQ(FILE* f);

	private:
		/* functions */
		void open_file(const char* filename, hid_t mode);
		void open_datasets(const char* path);
		void read_array_attribute(const char* name, hid_t type, hsize_t n, void* array);
		void create_file(const char* filename, hid_t mode);
		void create_datasets(const char* path);
		void write_array_attribute(const char* name, hid_t type, hsize_t n, const void* array);
		void flush_writes(hsize_t remainder);
		void flush_reads();
		void read_at(size_t i);
		void alloc();
		void clear_buffers();
		void clear_buffers_at();
		void export_block(FILE* f, hsize_t count);
		/* data */
		const size_t blocksize;
		/* buffers for block reading and writing */
		uint8_t* pbuf; // packed buffer
		size_t pbufsize;
		char* sbuf; // sequence (unpacked) buffer
		size_t sbufsize;
		char* ibuf; // ID buffer
		size_t ibufsize;
		/* buffers for random-access reads */
		uint8_t* pbuf_at;
		char* sbuf_at;
		char* ibuf_at;
		/* HDF5 objects */
		hid_t h5file;
		hid_t sdset;
		hid_t sspace;
		hid_t smemspace;
		hid_t idset;
		hid_t ispace;
		hid_t imemspace;
};

#endif

