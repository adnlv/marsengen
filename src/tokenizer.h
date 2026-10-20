#ifndef MARSENGEN_TOKENIZER_H
#define MARSENGEN_TOKENIZER_H

#include <stdbool.h>
#include <stdio.h>

#define DELIM ' '

typedef struct
{
    char *ptr;
    int len;
} token_t;

FILE *open_corpus_file(const char *path);
long get_corpus_file_len(FILE *corpus_file);
void format_corpus_text(FILE *corpus_file, char *buf, int *len_ptr);
int count_tokens(const char *str, int len);
bool tokenize_input_text(char *str,
                         int str_len,
                         token_t *tokens,
                         int n_tokens,
                         token_t *uniques,
                         int *n_uniques_ptr);

#endif /* MARSENGEN_TOKENIZER_H */
