#include "memory.h"

#include <assert.h>
#include <stdio.h>

int mem_open(FILE **stream_ptr, mem_header_t *header_ptr)
{
    assert(stream_ptr != NULL);
    assert(header_ptr != NULL);

    FILE *fp = fopen("memory.mem", "r");
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

void mem_close(FILE *stream) { fclose(stream); }
