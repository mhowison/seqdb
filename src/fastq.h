#ifndef __FASTQ_H__
#define __FASTQ_H__

#include <stdio.h>
#include <fstream>
#include "seq.h"

class FASTQ
{
	public:
		FASTQ(const char* filename);
		virtual bool good() = 0;
		virtual bool next(Sequence& seq) = 0;
		virtual void next_line(std::string& buf) = 0;
		virtual bool next_line(char* buf, size_t count) = 0;
		void check_delim(char delim);
		void check_plus();

	protected:
		const char* filename;
		size_t nline;
		std::string line;
};

class mmapFASTQ : public FASTQ
{
	public:
		mmapFASTQ(const char* filename);
		~mmapFASTQ();
		bool good();
		bool next(Sequence& seq);
		void next_line(std::string& buf);
		bool next_line(char* buf, size_t count);

	private:
		int fd;
		size_t size;
		size_t nchar;
		char* input;
};

class streamFASTQ : public FASTQ
{
	public:
		streamFASTQ(const char* filename);
		~streamFASTQ();
		bool good();
		bool next(Sequence& seq);
		void next_line(std::string& buf);
		bool next_line(char* buf, size_t count);

	private:
		std::ifstream* input_file;
		std::istream* input;
};

#endif

