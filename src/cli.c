#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *prog_name)
{
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("Markov chain sentence generator.\n\n");
    printf("Options:\n");
    printf("  -c, --corpus    <path>    Input text corpus file (default: input.txt)\n");
    printf("  -i, --input     <path>    Alias for -c, --corpus\n");
    printf("  -o, --output    <path>    Output destination file (default: output.txt)\n");
    printf("  -l, --log       <path>    Alias for -o, --output\n");
    printf("  -n, --sentences <count>   Number of sentences to generate (default: 10)\n");
    printf("  -w, --words     <count>   Maximum words per sentence (default: 50)\n");
    printf("  -s, --seed      <number>  Seed for random number generator (default: current time)\n");
    printf("  -v, --verbose             Display detailed processing statistics\n");
    printf("  -h, --help                Display this help message and exit\n");
}

bool parse_args(int argc, char **argv, cli_options_t *opts)
{
    opts->corpus_path = "input.txt";
    opts->output_path = "output.txt";
    opts->n_sentences = 10;
    opts->max_words = 50;
    opts->seed = 0;
    opts->has_seed = false;
    opts->verbose = false;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--corpus") == 0 ||
                 strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: Option '%s' requires a file path.\n\n", argv[i]);
                print_usage(argv[0]);
                return false;
            }
            opts->corpus_path = argv[++i];
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0 ||
                 strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: Option '%s' requires a file path.\n\n", argv[i]);
                print_usage(argv[0]);
                return false;
            }
            opts->output_path = argv[++i];
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--sentences") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: Option '%s' requires a positive integer.\n\n", argv[i]);
                print_usage(argv[0]);
                return false;
            }
            int val = atoi(argv[++i]);
            if (val <= 0)
            {
                fprintf(stderr, "Error: Number of sentences must be greater than zero.\n\n");
                return false;
            }
            opts->n_sentences = val;
        }
        else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--words") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: Option '%s' requires a positive integer.\n\n", argv[i]);
                print_usage(argv[0]);
                return false;
            }
            int val = atoi(argv[++i]);
            if (val <= 0)
            {
                fprintf(stderr, "Error: Maximum words per sentence must be greater than zero.\n\n");
                return false;
            }
            opts->max_words = val;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seed") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: Option '%s' requires a non-negative integer.\n\n", argv[i]);
                print_usage(argv[0]);
                return false;
            }
            opts->seed = (unsigned int)strtoul(argv[++i], NULL, 10);
            opts->has_seed = true;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            opts->verbose = true;
        }
        else
        {
            fprintf(stderr, "Error: Unrecognized option '%s'.\n\n", argv[i]);
            print_usage(argv[0]);
            return false;
        }
    }

    return true;
}
