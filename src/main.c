#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DELIM ' '
#define IS_LOWER(ch) ((ch) >= 'a' && (ch) <= 'z')
#define IS_UPPER(ch) ((ch) >= 'A' && (ch) <= 'Z')

static FILE *corpus_file = NULL;
static FILE *output_file = NULL;

bool open_corpus_file(const char *path)
{
    if (path == NULL)
    {
        fprintf(stderr, "Error: Corpus file path is NULL\n");
        return false;
    }

    corpus_file = fopen(path, "r");
    if (corpus_file == NULL)
    {
        fprintf(stderr, "Error: Failed to open corpus file \"%s\": %s\n", path, strerror(errno));
        return false;
    }

    return true;
}

bool open_output_file(const char *path)
{
    if (path == NULL)
    {
        fprintf(stderr, "Error: Output file path is NULL\n");
        return false;
    }

    output_file = fopen(path, "w+");
    if (output_file == NULL)
    {
        fprintf(stderr, "Error: Failed to open output file \"%s\": %s\n", path, strerror(errno));
        return false;
    }

    return true;
}

long get_corpus_file_len(void)
{
    if (corpus_file == NULL)
    {
        return -1;
    }

    if (fseek(corpus_file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Error: Failed to seek to end of corpus file: %s\n", strerror(errno));
        return -1;
    }

    const long pos = ftell(corpus_file);
    if (pos < 0)
    {
        fprintf(stderr, "Error: Failed to determine corpus file length: %s\n", strerror(errno));
        return -1;
    }

    if (fseek(corpus_file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Error: Failed to rewind corpus file: %s\n", strerror(errno));
        return -1;
    }

    return pos;
}

void format_corpus_text(char *buf, int *len_ptr)
{
    int len = 0;
    bool is_prev_letter = false;
    for (int i = 0; i < *len_ptr; ++i)
    {
        int c = getc(corpus_file);
        if (c == EOF)
        {
            break;
        }

        char ch = (char)c;
        bool is_lower = IS_LOWER(ch);
        bool is_upper = IS_UPPER(ch);
        if (is_lower || is_upper)
        {
            if (is_upper)
            {
                ch += 32;
            }

            buf[len] = ch;
            is_prev_letter = true;
            ++len;
        }
        else if (is_prev_letter)
        {
            is_prev_letter = false;
            buf[len++] = DELIM;
        }
    }

    if (len > 0)
    {
        if (buf[len - 1] == DELIM)
        {
            buf[len - 1] = '\0';
            --len;
        }
        else
        {
            buf[len] = '\0';
        }
    }
    else
    {
        buf[0] = '\0';
    }

    *len_ptr = len;
}

typedef struct
{
    char *ptr;
    int len;
} token_t;

int count_tokens(const char *str, const int len)
{
    if (str == NULL || len <= 0)
    {
        return 0;
    }

    int n_tokens = 1;
    for (int i = 0; i < len; ++i)
    {
        if (str[i] == DELIM)
        {
            ++n_tokens;
        }
    }

    return n_tokens;
}

static token_t *g_sort_tokens = NULL;

int token_index_cmp(const void *a, const void *b)
{
    int a_idx = *(const int *)a;
    int b_idx = *(const int *)b;
    token_t *a_tok = &g_sort_tokens[a_idx];
    token_t *b_tok = &g_sort_tokens[b_idx];

    int min_len = a_tok->len < b_tok->len ? a_tok->len : b_tok->len;
    int cmp = memcmp(a_tok->ptr, b_tok->ptr, min_len);
    if (cmp != 0)
    {
        return cmp;
    }

    return a_tok->len - b_tok->len;
}

bool tokenize_input_text(char *str,
                         int str_len,
                         token_t *tokens,
                         int n_tokens,
                         token_t *uniques,
                         int *n_uniques_ptr)
{
    if (str == NULL || str_len <= 0 || tokens == NULL || n_tokens <= 0 ||
        uniques == NULL || n_uniques_ptr == NULL)
    {
        return false;
    }

    int str_idx = 0;
    char *cur_ptr = str;
    char *prev_ptr = cur_ptr;
    for (int i = 0; i < n_tokens; ++i)
    {
        tokens[i].ptr = cur_ptr;

        while (*cur_ptr != DELIM && *cur_ptr != '\0' && str_idx < str_len)
        {
            ++cur_ptr;
            ++str_idx;
        }

        tokens[i].len = (int)(cur_ptr - prev_ptr);
        prev_ptr = ++cur_ptr;
        ++str_idx;
    }

    int *idxs = malloc(sizeof(int) * n_tokens);
    if (idxs == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for token sorting indices\n");
        return false;
    }

    for (int i = 0; i < n_tokens; ++i)
    {
        idxs[i] = i;
    }

    g_sort_tokens = tokens;
    qsort(idxs, n_tokens, sizeof(int), token_index_cmp);

    char *canon = tokens[idxs[0]].ptr;
    uniques[0] = tokens[idxs[0]];
    int unique_idx = 1;
    for (int i = 1; i < n_tokens; ++i)
    {
        token_t *cur = &tokens[idxs[i]];
        token_t *prev = &tokens[idxs[i - 1]];
        if (cur->len == prev->len && memcmp(cur->ptr, prev->ptr, cur->len) == 0)
        {
            cur->ptr = canon;
        }
        else
        {
            canon = cur->ptr;
            uniques[unique_idx++] = *cur;
        }
    }

    *n_uniques_ptr = unique_idx;

    free(idxs);

    return true;
}

typedef struct
{
    token_t *fst;
    token_t *sec;
} bigram_t;

int count_bigrams(const int n_tokens)
{
    return n_tokens > 1 ? n_tokens - 1 : 0;
}

void generate_bigrams_from_tokens(token_t *tokens,
                                  bigram_t *bigrams,
                                  const int n_bigrams)
{
    for (int i = 0; i < n_bigrams; ++i)
    {
        bigrams[i].fst = &tokens[i];
        bigrams[i].sec = &tokens[i + 1];
    }
}

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

typedef struct
{
    char *ptr;
    int idx;
} ptr_idx_t;

int ptr_idx_cmp(const void *a, const void *b)
{
    const ptr_idx_t *a_ptr = (const ptr_idx_t *)a;
    const ptr_idx_t *b_ptr = (const ptr_idx_t *)b;

    if (a_ptr->ptr < b_ptr->ptr)
    {
        return -1;
    }

    if (a_ptr->ptr > b_ptr->ptr)
    {
        return 1;
    }

    return 0;
}

int find_token_idx(ptr_idx_t *map, int n, char *ptr)
{
    int low = 0;
    int high = n - 1;
    while (low <= high)
    {
        int mid = low + (high - low) / 2;
        if (map[mid].ptr == ptr)
        {
            return map[mid].idx;
        }

        if (map[mid].ptr < ptr)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    return -1;
}

bool build_adjacency_list(bigram_t *bigrams,
                          int n_bigrams,
                          token_t *uniques,
                          int n_uniques,
                          word_transitions_t *transitions)
{
    if (n_uniques <= 0 || transitions == NULL)
    {
        return false;
    }

    for (int i = 0; i < n_uniques; ++i)
    {
        transitions[i].items = NULL;
        transitions[i].len = 0;
        transitions[i].cap = 0;
        transitions[i].total_count = 0;
    }

    if (n_bigrams <= 0)
    {
        return true;
    }

    ptr_idx_t *map = malloc(sizeof(ptr_idx_t) * n_uniques);
    if (map == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for token pointer map\n");
        return false;
    }

    for (int i = 0; i < n_uniques; ++i)
    {
        map[i].ptr = uniques[i].ptr;
        map[i].idx = i;
    }

    qsort(map, n_uniques, sizeof(ptr_idx_t), ptr_idx_cmp);

    for (int k = 0; k < n_bigrams; ++k)
    {
        int src = find_token_idx(map, n_uniques, bigrams[k].fst->ptr);
        int dst = find_token_idx(map, n_uniques, bigrams[k].sec->ptr);
        if (src == -1 || dst == -1)
        {
            continue;
        }

        word_transitions_t *wt = &transitions[src];
        int found = -1;
        for (int i = 0; i < wt->len; ++i)
        {
            if (wt->items[i].next_uniq_idx == dst)
            {
                found = i;
                break;
            }
        }

        if (found >= 0)
        {
            wt->items[found].count++;
        }
        else
        {
            if (wt->len == wt->cap)
            {
                int new_cap = wt->cap == 0 ? 4 : wt->cap * 2;
                transition_t *new_items = realloc(wt->items, sizeof(transition_t) * new_cap);
                if (new_items == NULL)
                {
                    fprintf(stderr, "Error: Failed to reallocate memory for transitions\n");
                    free(map);
                    return false;
                }
                wt->items = new_items;
                wt->cap = new_cap;
            }

            wt->items[wt->len].next_uniq_idx = dst;
            wt->items[wt->len].count = 1;
            wt->len++;
        }

        ++wt->total_count;
    }

    free(map);

    return true;
}

void free_transitions(word_transitions_t *transitions, const int n_uniques)
{
    if (transitions == NULL)
    {
        return;
    }

    for (int i = 0; i < n_uniques; ++i)
    {
        free(transitions[i].items);
    }

    free(transitions);
}

void generate_sentences(token_t *uniques,
                        const int n_uniques,
                        word_transitions_t *transitions)
{
    if (uniques == NULL || n_uniques <= 0 || transitions == NULL)
    {
        return;
    }

    srand((unsigned)time(NULL));

    const int n_sentences = 32;
    const int n_words_per_sentence = n_uniques < 0xFF ? n_uniques : 0xFF;

    for (int sentences = 0; sentences < n_sentences; ++sentences)
    {
        token_t words[n_words_per_sentence];
        int words_len = 1;

        int word_idx = rand() % n_uniques;
        words[0] = uniques[word_idx];

        while (words_len < n_words_per_sentence)
        {
            word_transitions_t *wt = &transitions[word_idx];
            if (wt->len == 0)
            {
                word_idx = rand() % n_uniques;
                words[words_len++] = uniques[word_idx];
                continue;
            }

            int r = rand() % wt->total_count;
            int cumulative = 0;
            int chosen = wt->items[wt->len - 1].next_uniq_idx;
            for (int i = 0; i < wt->len; ++i)
            {
                cumulative += wt->items[i].count;
                if (r < cumulative)
                {
                    chosen = wt->items[i].next_uniq_idx;
                    break;
                }
            }

            word_idx = chosen;
            words[words_len++] = uniques[word_idx];
        }

        fputs("Sentence: \"", output_file);
        for (int i = 0; i < words_len; ++i)
        {
            for (int j = 0; j < words[i].len; ++j)
            {
                fputc(words[i].ptr[j], output_file);
            }

            if (i != words_len - 1)
            {
                fputc(' ', output_file);
            }
        }

        fputs("\"\n", output_file);
    }
}

typedef struct
{
    const char *corpus_path;
    const char *output_path;
} cli_options_t;

static void print_usage(const char *prog_name)
{
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("Markov chain sentence generator.\n\n");
    printf("Options:\n");
    printf("  -c, --corpus <path>   Input text corpus file (default: input.txt)\n");
    printf("  -i, --input  <path>   Alias for -c, --corpus\n");
    printf("  -o, --output <path>   Output file for logs and sentences (default: output.txt)\n");
    printf("  -l, --log    <path>   Alias for -o, --output\n");
    printf("  -h, --help            Display this help message and exit\n");
}

static bool parse_args(int argc, char **argv, cli_options_t *opts)
{
    opts->corpus_path = "input.txt";
    opts->output_path = "output.txt";

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
        else
        {
            fprintf(stderr, "Error: Unrecognized option '%s'.\n\n", argv[i]);
            print_usage(argv[0]);
            return false;
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    cli_options_t opts;
    if (!parse_args(argc, argv, &opts))
    {
        return EXIT_FAILURE;
    }

    if (!open_corpus_file(opts.corpus_path))
    {
        return EXIT_FAILURE;
    }

    if (!open_output_file(opts.output_path))
    {
        fclose(corpus_file);
        return EXIT_FAILURE;
    }

    long file_len = get_corpus_file_len();
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

    format_corpus_text(text, &text_len);
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

    token_t *tokens = malloc(sizeof(token_t) * n_tokens);
    token_t *unique_tokens = malloc(sizeof(token_t) * n_tokens);
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
        bigrams = malloc(sizeof(bigram_t) * n_bigrams);
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

    word_transitions_t *transitions = calloc(n_unique_tokens,
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

    generate_sentences(unique_tokens, n_unique_tokens, transitions);

    free_transitions(transitions, n_unique_tokens);
    free(bigrams);
    free(unique_tokens);
    free(tokens);
    free(text);

    fclose(output_file);

    return EXIT_SUCCESS;
}
