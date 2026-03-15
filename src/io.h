#ifndef IO_H_
#define IO_H_

#include <stdint.h>
#include <stdio.h>

typedef struct
{
    char *buf;
    FILE *stream;
    uint64_t len;
} corpus_t;

FILE *fopen_output(const char *path);
FILE *fopen_corpus(const char *path);

#endif
