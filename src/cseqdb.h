#include <stdlib.h>
#include <stdint.h>

typedef uint32_t seqdb_file_t;

#ifdef __cplusplus
#define EXT extern "C"
#else
#define EXT  
#endif

EXT seqdb_file_t seqdb_open(const char* filename);
EXT void seqdb_close(seqdb_file_t f);
EXT int seqdb_next_record(seqdb_file_t f);
EXT size_t seqdb_id_len(seqdb_file_t f);
EXT void seqdb_id(seqdb_file_t f, char* id);
EXT size_t seqdb_seq_len(seqdb_file_t f);
EXT void seqdb_seq(seqdb_file_t f, char* seq);

