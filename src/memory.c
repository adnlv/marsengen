#include "memory.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int mem_open(FILE **stream_ptr, mem_header_t *header_ptr)
{
    assert(stream_ptr != NULL);
    assert(header_ptr != NULL);

    FILE *fp = fopen("memory.mem", "rb");
    if (fp == NULL)
    {
        return EMEM_NOMEM;
    }

    int count = fread(&header_ptr->magic, sizeof(header_ptr->magic), 1, fp);
    if (count != 1 || header_ptr->magic != MAGIC)
    {
        fclose(fp);

        return EMEM_NOMAGIC;
    }

    count = fread(&header_ptr->version, sizeof(header_ptr->version), 1, fp);
    if (count != 1 || header_ptr->version != VERSION)
    {
        fclose(fp);

        return EMEM_INCOMPAT;
    }

    *stream_ptr = fp;

    return 0;
}

void mem_close(FILE *stream)
{
    assert(stream != NULL);

    fclose(stream);
}

int mem_read_vocab(FILE *stream, mem_vocab_t *vocab_ptr)
{
    assert(stream != NULL);
    assert(vocab_ptr != NULL);

    int count = fread(&vocab_ptr->total, sizeof(vocab_ptr->total), 1, stream);
    if (count != 1 || vocab_ptr->total == 0)
    {
        return EMEM_INCOMPAT;
    }

    vocab_ptr->words = calloc(vocab_ptr->total, sizeof(mem_word_t));
    if (vocab_ptr->words == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < vocab_ptr->total; ++i)
    {
        mem_word_t *word = &vocab_ptr->words[i];

        count = fread(&word->len, sizeof(word->len), 1, stream);
        if (count != 1)
        {
            free(vocab_ptr->words);

            return EMEM_INCOMPAT;
        }

        word->str = calloc(word->len, sizeof(char));
        if (word->str == NULL)
        {
            free(vocab_ptr->words);

            return -1;
        }

        count = fread(word->str, sizeof(char), word->len, stream);
        if (count != word->len)
        {
            free(vocab_ptr->words);

            return EMEM_INCOMPAT;
        }
    }

    return 0;
}

void mem_free_vocab(mem_vocab_t *vocab_ptr)
{
    assert(vocab_ptr != NULL);

    for (uint32_t i = 0; i < vocab_ptr->total; ++i)
    {
        free(vocab_ptr->words[i].str);
    }

    free(vocab_ptr->words);
}

int mem_read_trans(FILE *stream, mem_trans_t *trans_ptr)
{
    assert(stream != NULL);
    assert(trans_ptr != NULL);

    int count = fread(&trans_ptr->n_distinct,
                      sizeof(trans_ptr->n_distinct),
                      1,
                      stream);
    if (count != 1)
    {
        return EMEM_INCOMPAT;
    }

    count
        = fread(&trans_ptr->total_obs, sizeof(trans_ptr->total_obs), 1, stream);
    if (count != 1)
    {
        return EMEM_INCOMPAT;
    }

    if (trans_ptr->n_distinct == 0)
    {
        return 0;
    }

    trans_ptr->dests = calloc(trans_ptr->n_distinct, sizeof(mem_tdest_t));
    if (trans_ptr->dests == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < trans_ptr->n_distinct; ++i)
    {
        mem_tdest_t *d = &trans_ptr->dests[i];

        count = fread(&d->idx, sizeof(d->idx), 1, stream);
        if (count != 1)
        {
            free(trans_ptr->dests);

            return EMEM_INCOMPAT;
        }

        count = fread(&d->count, sizeof(d->count), 1, stream);
        if (count != 1)
        {
            free(trans_ptr->dests);

            return EMEM_INCOMPAT;
        }
    }

    return 0;
}

void mem_free_trans(mem_trans_t *trans_ptr)
{
    assert(trans_ptr != NULL);

    free(trans_ptr->dests);
}

int mem_save(FILE *stream, mem_vocab_t *vocab, mem_trans_t *trans)
{
    bool opened_manually = false;
    if (stream == NULL)
    {
        stream = fopen("memory.mem", "w+b");
        if (stream == NULL)
        {
            return -1;
        }

        opened_manually = true;
    }

    assert(vocab != NULL);
    assert(trans != NULL);

    if (fseek(stream, 0, SEEK_SET) != 0)
    {
        return -1;
    }

    uint32_t magic = MAGIC;
    if (fwrite(&magic, sizeof(magic), 1, stream) != 1)
    {
        return -1;
    }

    uint16_t version = VERSION;
    if (fwrite(&version, sizeof(version), 1, stream) != 1)
    {
        return -1;
    }

    if (fwrite(&vocab->total, sizeof(vocab->total), 1, stream) != 1)
    {
        return -1;
    }

    for (uint32_t i = 0; i < vocab->total; ++i)
    {
        mem_word_t *word = &vocab->words[i];
        if (fwrite(&word->len, sizeof(word->len), 1, stream) != 1)
        {
            return -1;
        }

        if (fwrite(word->str, sizeof(char), word->len, stream) != word->len)
        {
            return -1;
        }
    }

    if (fwrite(&trans->n_distinct, sizeof(trans->n_distinct), 1, stream) != 1)
    {
        return -1;
    }

    if (fwrite(&trans->total_obs, sizeof(trans->total_obs), 1, stream) != 1)
    {
        return -1;
    }

    for (uint32_t i = 0; i < trans->n_distinct; ++i)
    {
        mem_tdest_t *dest = &trans->dests[i];
        if (fwrite(&dest->idx, sizeof(dest->idx), 1, stream) != 1)
        {
            return -1;
        }

        if (fwrite(&dest->count, sizeof(dest->count), 1, stream) != 1)
        {
            return -1;
        }
    }

    if (opened_manually)
    {
        fclose(stream);
    }

    return 0;
}
