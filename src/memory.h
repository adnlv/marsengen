#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>
#include <stdio.h>

#define MAGIC 0x4D52534E
#define VERSION 0x0001

/* Memory file header */
typedef struct
{
    uint32_t magic;
    uint16_t version;
} mem_header_t;

int mem_open(FILE **stream_ptr, mem_header_t *header_ptr);
void mem_close(FILE *stream);

/* Individual vocabulary word */
typedef struct
{
    char *str;
    uint8_t len;
} mem_word_t;

/* Unique words vocabulary */
typedef struct
{
    mem_word_t *words;
    uint32_t total;
} mem_vocab_t;

int mem_read_vocab(FILE *stream, mem_vocab_t *vocab_ptr);
void mem_free_vocab(mem_vocab_t *vocab_ptr);

/* Individual transition destination */
typedef struct
{
    uint32_t idx; /* unique word vocabulary index */
    uint32_t count;
} mem_tdest_t;

/* Word transitions */
typedef struct
{
    uint32_t n_distinct; /* number of distinct next-words */
    uint32_t total_obs; /* total observation count */
    mem_tdest_t *dests;
} mem_trans_t;

int mem_read_trans(FILE *stream, mem_trans_t *trans_ptr);
void mem_free_trans(mem_trans_t *trans_ptr);

#endif /* MEMORY_H_ */
