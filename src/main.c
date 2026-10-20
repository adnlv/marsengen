#include "cli.h"
#include "markov.h"
#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    cli_options_t opts;
    if (!parse_args(argc, argv, &opts))
    {
        return EXIT_FAILURE;
    }

    FILE *corpus_file = open_corpus_file(opts.corpus_path);
    if (corpus_file == NULL)
    {
        return EXIT_FAILURE;
    }

    FILE *output_file = open_output_file(opts.output_path);
    if (output_file == NULL)
    {
        fclose(corpus_file);
        return EXIT_FAILURE;
    }

    long file_len = get_corpus_file_len(corpus_file);
    if (file_len < 0)
    {
        fprintf(stderr, "Error: Failed to determine corpus file size\n");
        fclose(corpus_file);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    if (file_len == 0)
    {
        fprintf(stderr, "Error: Corpus file is empty\n");
        fclose(corpus_file);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    int text_len = (int)file_len;
    char *text = calloc((size_t)text_len + 1, 1);
    if (text == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for corpus text\n");
        fclose(corpus_file);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    format_corpus_text(corpus_file, text, &text_len);
    fclose(corpus_file);

    if (text_len <= 0)
    {
        fprintf(stderr, "Error: Corpus contains no valid word tokens\n");
        free(text);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    int n_tokens = count_tokens(text, text_len);
    if (n_tokens <= 0)
    {
        fprintf(stderr, "Error: Failed to parse tokens from corpus\n");
        free(text);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    token_t *tokens = malloc(sizeof(token_t) * (size_t)n_tokens);
    token_t *unique_tokens = malloc(sizeof(token_t) * (size_t)n_tokens);
    if (tokens == NULL || unique_tokens == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for tokens\n");
        free(tokens);
        free(unique_tokens);
        free(text);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    int n_unique_tokens = 0;
    if (!tokenize_input_text(text,
                            text_len,
                            tokens,
                            n_tokens,
                            unique_tokens,
                            &n_unique_tokens))
    {
        fprintf(stderr, "Error: Tokenization failed\n");
        free(unique_tokens);
        free(tokens);
        free(text);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    int n_bigrams = count_bigrams(n_tokens);
    bigram_t *bigrams = NULL;
    if (n_bigrams > 0)
    {
        bigrams = malloc(sizeof(bigram_t) * (size_t)n_bigrams);
        if (bigrams == NULL)
        {
            fprintf(stderr, "Error: Failed to allocate memory for bigrams\n");
            free(unique_tokens);
            free(tokens);
            free(text);
            fclose(output_file);
            return EXIT_FAILURE;
        }

        generate_bigrams_from_tokens(tokens, bigrams, n_bigrams);
    }

    word_transitions_t *transitions = calloc((size_t)n_unique_tokens,
                                             sizeof(word_transitions_t));
    if (transitions == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for transitions\n");
        free(bigrams);
        free(unique_tokens);
        free(tokens);
        free(text);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    if (!build_adjacency_list(bigrams,
                             n_bigrams,
                             unique_tokens,
                             n_unique_tokens,
                             transitions))
    {
        fprintf(stderr, "Error: Failed to build adjacency list\n");
        free_transitions(transitions, n_unique_tokens);
        free(bigrams);
        free(unique_tokens);
        free(tokens);
        free(text);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    if (opts.verbose)
    {
        printf("Corpus: \"%s\" (%ld characters)\n", opts.corpus_path, file_len);
        printf("Formatted text: %d characters, %d tokens, %d unique vocabulary\n",
               text_len, n_tokens, n_unique_tokens);
        printf("Bigrams: %d\n", n_bigrams);
    }

    generate_sentences(output_file, unique_tokens, n_unique_tokens, transitions, &opts);

    if (opts.verbose)
    {
        printf("Generated %d sentences to \"%s\"\n", opts.n_sentences, opts.output_path);
    }

    free_transitions(transitions, n_unique_tokens);
    free(bigrams);
    free(unique_tokens);
    free(tokens);
    free(text);

    fclose(output_file);

    return EXIT_SUCCESS;
}
