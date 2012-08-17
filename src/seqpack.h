#ifndef __SEQPACK_H__
#define __SEQPACK_H__

#include <stdlib.h>
#include <inttypes.h>

#define SEQPACK_ENC_SIZE 128
#define SEQPACK_DEC_SIZE 256

class SeqPack {

	public:
		SeqPack(int length);
		void setLength(int length) { this->length = length; }
		const uint8_t* getEncBase() { return enc_base; }
		const uint8_t* getEncQual() { return enc_qual; }
		const char* getDecBase() { return dec_base; }
		const char* getDecQual() { return dec_qual; }
		void pack(const char* seq, const char* qual, uint8_t* record);
		void parpack(size_t n, const char* src, uint8_t* dst);
		void unpack(const uint8_t* record, char* seq, char* qual);
		void parunpack(size_t n, const uint8_t* src, char* dst);
		void unpack_v2(const uint8_t* record, char* seq, char* qual);

	private:
		int length;
		int qual_offset;
		uint8_t enc_base[SEQPACK_ENC_SIZE];
		uint8_t enc_qual[SEQPACK_ENC_SIZE];
		char dec_base[SEQPACK_DEC_SIZE];
		char dec_qual[SEQPACK_DEC_SIZE];
};

#endif

