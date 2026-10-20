#include "markov.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

FILE *open_output_file(const char *path)
{
    if (path == NULL)
    {
        fprintf(stderr, "Error: Output file path is NULL\n");
        return NULL;
    }

    FILE *f = fopen(path, "w+");
    if (f == NULL)
    {
        fprintf(stderr, "Error: Failed to open output file \"%s\": %s\n", path, strerror(errno));
        return NULL;
    }

    return f;
}

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
    char *ptr;
    int idx;
} ptr_idx_t;

static int ptr_idx_cmp(const void *a, const void *b)
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

static int find_token_idx(ptr_idx_t *map, int n, char *ptr)
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

    ptr_idx_t *map = malloc(sizeof(ptr_idx_t) * (size_t)n_uniques);
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

    qsort(map, (size_t)n_uniques, sizeof(ptr_idx_t), ptr_idx_cmp);

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
                transition_t *new_items = realloc(wt->items, sizeof(transition_t) * (size_t)new_cap);
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

void generate_sentences(FILE *output_file,
                        token_t *uniques,
                        const int n_uniques,
                        word_transitions_t *transitions,
                        const cli_options_t *opts)
{
    if (output_file == NULL || uniques == NULL || n_uniques <= 0 ||
        transitions == NULL || opts == NULL)
    {
        return;
    }

    if (opts->has_seed)
    {
        srand(opts->seed);
    }
    else
    {
        srand((unsigned)time(NULL));
    }

    const int n_sentences = opts->n_sentences;
    const int max_limit = opts->max_words < n_uniques ? opts->max_words : n_uniques;
    const int words_cap = max_limit > 0 ? max_limit : 1;

    for (int sentences = 0; sentences < n_sentences; ++sentences)
    {
        token_t *words = malloc(sizeof(token_t) * (size_t)words_cap);
        if (words == NULL)
        {
            fprintf(stderr, "Error: Failed to allocate memory for sentence buffer\n");
            return;
        }

        int words_len = 1;
        int word_idx = rand() % n_uniques;
        words[0] = uniques[word_idx];

        while (words_len < words_cap)
        {
            word_transitions_t *wt = &transitions[word_idx];
            if (wt->len == 0)
            {
                break;
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
        free(words);
    }
}
