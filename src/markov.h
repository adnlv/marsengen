#ifndef MARSENGEN_MARKOV_H
#define MARSENGEN_MARKOV_H

#include "cli.h"
#include "tokenizer.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct
{
    token_t *fst;
    token_t *sec;
} bigram_t;

typedef struct
{
    int count;
    int next_uniq_idx;
} transition_t;

typedef struct
{
    transition_t *items;
    int len;
    int cap;
    int total_count;
} word_transitions_t;

FILE *open_output_file(const char *path);
int count_bigrams(int n_tokens);
void generate_bigrams_from_tokens(token_t *tokens, bigram_t *bigrams, int n_bigrams);
bool build_adjacency_list(bigram_t *bigrams,
                          int n_bigrams,
                          token_t *uniques,
                          int n_uniques,
                          word_transitions_t *transitions);
void free_transitions(word_transitions_t *transitions, int n_uniques);
void generate_sentences(FILE *output_file,
                        token_t *uniques,
                        int n_uniques,
                        word_transitions_t *transitions,
                        const cli_options_t *opts);

#endif /* MARSENGEN_MARKOV_H */
