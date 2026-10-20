#ifndef MARSENGEN_CLI_H
#define MARSENGEN_CLI_H

#include <stdbool.h>

typedef struct
{
    const char *corpus_path;
    const char *output_path;
    int n_sentences;
    int max_words;
    unsigned int seed;
    bool has_seed;
    bool verbose;
} cli_options_t;

void print_usage(const char *prog_name);
bool parse_args(int argc, char **argv, cli_options_t *opts);

#endif /* MARSENGEN_CLI_H */
