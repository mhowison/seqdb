#ifndef __SEQ_H__
#define __SEQ_H__

#include <iostream>
#include <fstream>
#include <string>

class Sequence
{
	public:

		Sequence() { }
		void clear();
		static void printFASTQ(
				FILE* f,
				const char* idline,
				const char* seq,
				const char* qual,
				size_t ilen,
				size_t slen);
		static void printFASTQ(
				FILE* f,
				std::string& idline,
				std::string& seq,
				std::string& qual);
		void print();
		friend std::ostream& operator << (std::ostream& o, const Sequence& s);

		std::string idline;
		std::string seq;
		std::string qual;
};

#endif

