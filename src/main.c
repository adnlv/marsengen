#include "io.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define DELIM ' '
#define IS_LOWER(ch) ((ch) >= 'a' && (ch) <= 'z')
#define IS_UPPER(ch) ((ch) >= 'A' && (ch) <= 'Z')

static FILE *output_stream = NULL;

int64_t get_corpus_len(FILE *corpus_stream)
{
    if (fseek(corpus_stream, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Error setting stream cursor: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    const int64_t pos = ftell(corpus_stream);
    if (pos <= 0)
    {
        fprintf(stderr,
                "Error calculating stream length: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fseek(corpus_stream, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Error setting stream cursor: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fprintf(output_stream, "Unformatted corpus contains %ld characters\n", pos);

    return pos;
}

void read_corpus_stream(FILE *corpus_stream, char *buf, uint64_t *len_ptr)
{
    int len = 0;
    bool is_prev_letter = false;
    for (int i = 0; i < *len_ptr; ++i)
    {
        int c = getc(corpus_stream);
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
        buf[len - 1] = '\0';
    }

    *len_ptr = len;

    fprintf(output_stream,
            "Formatted corpus contains %d characters: %s\n",
            len,
            buf);
}

typedef struct
{
    char *ptr;
    int len;
} token_t;

int count_tokens(const char *str, int len)
{
    int n_tokens = 1;
    for (int i = 0; i < len; ++i)
    {
        if (str[i] == DELIM)
        {
            ++n_tokens;
        }
    }

    fprintf(output_stream, "Corpus contains %d tokens\n", n_tokens);

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

void tokenize_text(char *str,
                   int str_len,
                   token_t *tokens,
                   int n_tokens,
                   token_t *uniques,
                   int *n_uniques_ptr)
{
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

        tokens[i].len = cur_ptr - prev_ptr;
        prev_ptr = ++cur_ptr;
        ++str_idx;
    }

    int *idxs = malloc(sizeof(int) * n_tokens);
    if (idxs == NULL)
    {
        fprintf(stderr,
                "Error allocating memory for token indexes: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
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

    for (int i = 0; i < n_tokens; ++i)
    {
        fprintf(output_stream, "Token [%d] \"", i);

        for (int j = 0; j < tokens[i].len; ++j)
        {
            fputc(tokens[i].ptr[j], output_stream);
        }

        fprintf(output_stream,
                "\" -> %p is %d characters long\n",
                (void *)tokens[i].ptr,
                tokens[i].len);
    }

    fprintf(output_stream,
            "corpus contains %d unique tokens\n",
            *n_uniques_ptr);

    for (int i = 0; i < *n_uniques_ptr; ++i)
    {
        fprintf(output_stream, "Unique token [%d] \"", i);

        for (int j = 0; j < uniques[i].len; ++j)
        {
            fputc(uniques[i].ptr[j], output_stream);
        }

        fprintf(output_stream,
                "\" -> %p is %d characters long\n",
                (void *)uniques[i].ptr,
                uniques[i].len);
    }
}

typedef struct
{
    token_t *fst;
    token_t *sec;
} bigram_t;

int count_bigrams(int n_tokens)
{
    const int n_2grams = n_tokens - 1;

    fprintf(output_stream, "Generating %d 2-grams\n", n_2grams);

    return n_2grams;
}

void generate_bigrams(token_t *tokens, bigram_t *bigrams, int n_bigrams)
{
    for (int i = 0; i < n_bigrams; ++i)
    {
        bigrams[i].fst = &tokens[i];
        bigrams[i].sec = &tokens[i + 1];
    }

    for (int i = 0; i < n_bigrams; ++i)
    {
        fprintf(output_stream, "2-Gram [%d] (\"", i);

        for (int j = 0; j < bigrams[i].fst->len; ++j)
        {
            fputc(bigrams[i].fst->ptr[j], output_stream);
        }

        fprintf(output_stream, "\" -> %p, \"", (void *)bigrams[i].fst);

        for (int j = 0; j < bigrams[i].sec->len; ++j)
        {
            fputc(bigrams[i].sec->ptr[j], output_stream);
        }

        fprintf(output_stream, "\" -> %p)\n", (void *)bigrams[i].sec);
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

void build_adjacency_list(bigram_t *bigrams,
                          int n_bigrams,
                          token_t *uniques,
                          int n_uniques,
                          word_transitions_t *transitions)
{
    ptr_idx_t *map = malloc(sizeof(ptr_idx_t) * n_uniques);
    if (map == NULL)
    {
        fprintf(stderr,
                "Error allocating memory for indexes map: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n_uniques; ++i)
    {
        map[i].ptr = uniques[i].ptr;
        map[i].idx = i;
    }

    qsort(map, n_uniques, sizeof(ptr_idx_t), ptr_idx_cmp);

    for (int i = 0; i < n_uniques; ++i)
    {
        transitions[i].items = NULL;
        transitions[i].len = 0;
        transitions[i].cap = 0;
        transitions[i].total_count = 0;
    }

    for (int i = 0; i < n_bigrams; ++i)
    {
        int src = find_token_idx(map, n_uniques, bigrams[i].fst->ptr);
        int dst = find_token_idx(map, n_uniques, bigrams[i].sec->ptr);
        if (src == -1 && dst == -1)
        {
            fprintf(stderr,
                    "Either source or destination token must be found\n");
            exit(EXIT_FAILURE);
        }

        word_transitions_t *wt = &transitions[src];
        int found = -1;
        for (int j = 0; j < wt->len; ++j)
        {
            if (wt->items[j].next_uniq_idx == dst)
            {
                found = j;
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
                wt->cap = wt->cap == 0 ? 4 : wt->cap * 2;
                wt->items = realloc(wt->items, sizeof(transition_t) * wt->cap);
                if (wt->items == NULL)
                {
                    fprintf(
                        stderr,
                        "Error reallocating memory for word transitions: %s\n",
                        strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }

            wt->items[wt->len].next_uniq_idx = dst;
            wt->items[wt->len].count = 1;
            wt->len++;
        }

        ++wt->total_count;
    }

    free(map);

    for (int i = 0; i < n_uniques; ++i)
    {
        word_transitions_t t = transitions[i];
        fprintf(output_stream,
                "Word [%d] has %d out of %d transitions\n",
                i,
                t.len,
                t.total_count);
    }
}

void generate_sentences(token_t *uniques,
                        int n_uniques,
                        word_transitions_t *transitions)
{
    srand((unsigned)time(NULL));

    const int n_sentences = 32;
    const int n_words_per_sentence = n_uniques < 0xFF ? n_uniques : 0xFF;

    fprintf(output_stream, "Generating %d sentences\n", n_sentences);

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

        fputs("Sentence: \"", output_stream);
        for (int i = 0; i < words_len; ++i)
        {
            for (int j = 0; j < words[i].len; ++j)
            {
                fputc(words[i].ptr[j], output_stream);
            }

            if (i != words_len - 1)
            {
                fputc(' ', output_stream);
            }
        }

        fputs("\"\n", output_stream);
    }
}

static void app_open_corpus(const char *path, corpus_t *corp_ptr)
{
    corp_ptr->stream = fopen_corpus(path);
    if (corp_ptr->stream == NULL)
    {
        fprintf(stderr, "Error opening corpus stream: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    corp_ptr->len = get_corpus_len(corp_ptr->stream);
    corp_ptr->buf = calloc(corp_ptr->len, 1);
    if (corp_ptr->buf == NULL)
    {
        fprintf(stderr,
                "Error allocating memory for corpus buffer: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    read_corpus_stream(corp_ptr->stream, corp_ptr->buf, &corp_ptr->len);
    fclose(corp_ptr->stream);
}

static void app_free_corpus(corpus_t *corp) { free(corp->buf); }

int main(int argc, char **argv)
{
    char *output_path = NULL;
    char *corp_path = NULL;
    for (int i = 1; i < argc; ++i)
    {
        char *s = argv[i];
        bool has_next = i + 1 < argc;

        if (has_next && (strcmp(s, "-l") == 0 || strcmp(s, "--log") == 0))
        {
            output_path = argv[++i];
        }
        else if (has_next
                 && (strcmp(s, "-c") == 0 || strcmp(s, "--corpus") == 0))
        {
            corp_path = argv[++i];
        }
    }

    output_stream = fopen_output(output_path);
    if (output_stream == NULL)
    {
        fprintf(stderr, "Error opening output stream: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    corpus_t corp;
    app_open_corpus(corp_path, &corp);

    int n_tokens = count_tokens(corp_path, corp.len);
    token_t *tokens = calloc(n_tokens, sizeof(token_t));
    if (tokens == NULL)
    {
        fprintf(stderr,
                "Error allocating memory for tokens: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    int n_unique_tokens = 0;
    token_t *unique_tokens = calloc(n_tokens, sizeof(token_t));
    if (tokens == NULL)
    {
        fprintf(stderr,
                "Error allocating memory for unique tokens: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    tokenize_text(corp.buf,
                  corp.len,
                  tokens,
                  n_tokens,
                  unique_tokens,
                  &n_unique_tokens);

    int n_bigrams = count_bigrams(n_tokens);
    bigram_t *bigrams = malloc(sizeof(bigram_t) * n_bigrams);
    if (bigrams == NULL)
    {
        fprintf(stderr,
                "Error allocating memory for bigrams: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    generate_bigrams(tokens, bigrams, n_bigrams);

    word_transitions_t *transitions = calloc(n_unique_tokens,
                                             sizeof(word_transitions_t));
    if (bigrams == NULL)
    {
        fprintf(stderr,
                "Error allocating memory for transitions: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    build_adjacency_list(bigrams,
                         n_bigrams,
                         unique_tokens,
                         n_unique_tokens,
                         transitions);

    generate_sentences(unique_tokens, n_unique_tokens, transitions);

    app_free_corpus(&corp);

    free(unique_tokens);
    free(bigrams);
    free(tokens);

    fclose(output_stream);

    return 0;
}
