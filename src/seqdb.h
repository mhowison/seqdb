#ifndef __SEQDB_H__
#define __SEQDB_H__

#include <stdio.h>
#include <string>
#include "seqpack.h"
#include "seq.h"
#include "fastq.h"

class SeqDB {

	public:
		/* functions */
		SeqDB(const char* filename, char mode, size_t slen, size_t ilen);
		static SeqDB* open(const char* filename);
		static SeqDB* create(const char* filename, char mode, size_t slen, size_t ilen);
		size_t size() { return nrecords; }
		size_t getSeqLength() { return slen; }
		size_t getIDLength() { return ilen; }

		/* virtual functions */
		virtual ~SeqDB();
		virtual void write(const Sequence& seq);
		virtual bool read(Sequence& seq);
		virtual void readAt(size_t i, Sequence& seq);
		virtual void importFASTQ(FASTQ* f);
		virtual void exportFASTQ(FILE* f);

		/* data */
		static const char READ = 0x00;
		static const char TRUNCATE = 0x01;
		static const char APPEND = 0x02;

	protected:
		/* data */
		const char* filename;
		size_t nrecords;
		size_t nread;
		size_t slen;
		size_t ilen;
		int qual_offset;
		char mode;
		SeqPack* pack;
};

#endif

