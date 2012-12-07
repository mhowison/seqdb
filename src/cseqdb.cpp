#include <string.h>
#include <vector>
#include "seqdb.h"
#include "cseqdb.h"

static std::vector<SeqDB*> dbs;
static std::vector<Sequence*> seqs;

seqdb_file_t seqdb_open(const char* filename)
{
	seqdb_file_t ret = dbs.size();
	dbs.push_back(SeqDB::open(filename));
	seqs.push_back(new Sequence());
	return ret;
}

void seqdb_close(seqdb_file_t f)
{
	delete dbs[f];
	delete seqs[f];
}

int seqdb_next_record(seqdb_file_t f)
{
	return (int)dbs[f]->read(*seqs[f]);
}

size_t seqdb_id_len(seqdb_file_t f)
{
	return seqs[f]->idline.size();
}

void seqdb_id(seqdb_file_t f, char* id)
{
	size_t len = seqs[f]->idline.size();
	strncpy(id, seqs[f]->idline.data(), len);
}

size_t seqdb_seq_len(seqdb_file_t f)
{
	return seqs[f]->seq.size();
}

void seqdb_seq(seqdb_file_t f, char* seq)
{
	size_t len = seqs[f]->seq.size();
	strncpy(seq, seqs[f]->seq.data(), len);
}
