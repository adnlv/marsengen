#include "memory.h"
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

static FILE *corpus_stream = NULL;
static FILE *out_stream = NULL;

void open_corpus_stream(char *path)
{
    corpus_stream = fopen(path != NULL ? path : "corpus.txt", "r");
    if (corpus_stream == NULL)
    {
        fprintf(stderr, "Error opening corpus: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Reading corpus \"%s\"\n", path);
}

void mkdir_if_not_exists(const char *path)
{
    int result = mkdir(path, 0755);
    if (result == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Error creating directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void open_out_stream(char *path)
{
    char *buf = path;
    if (buf == NULL)
    {
        /* path length = directory path ("logs" - 4)
         * + maximum timestamp length (20 for 64-bit machines)
         * + file extension (".log" - 4)
         * + null terminator (1)
         */
        const int len = 29;
        const char dir[] = "logs";
        const time_t secs = time(NULL);

        buf = calloc(len, sizeof(char));
        assert(buf != NULL);

        mkdir_if_not_exists(dir);
        snprintf(buf, len, "%s/%lu.log", dir, (unsigned long)secs);
    }

    out_stream = fopen(buf, "w+");
    assert(out_stream != NULL);

    printf("Writing output to \"%s\"\n", buf);

    if (path == NULL)
    {
        free(buf);
    }
}

int get_corpus_len(void)
{
    assert(fseek(corpus_stream, 0, SEEK_END) == 0);

    const long pos = ftell(corpus_stream);

    assert(pos != -1);
    assert(fseek(corpus_stream, 0, SEEK_SET) == 0);

    fprintf(out_stream, "Unformatted text contains %ld characters\n", pos);

    return pos;
}

void format_corpus_text(char *buf, int *len_ptr)
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

    fprintf(out_stream,
            "Formatted text contains %d characters: %s\n",
            len,
            buf);
}

int count_words(const char *str, int len)
{
    assert(str != NULL);
    assert(len > 0);

    int n_words = 1;
    for (int i = 0; i < len; ++i)
    {
        if (str[i] == DELIM)
        {
            ++n_words;
        }
    }

    fprintf(out_stream, "Text contains %d words\n", n_words);

    return n_words;
}

static mem_word_t *g_sort_words = NULL;

int word_index_cmp(const void *a, const void *b)
{
    int a_idx = *(const int *)a;
    int b_idx = *(const int *)b;
    mem_word_t *a_word = &g_sort_words[a_idx];
    mem_word_t *b_word = &g_sort_words[b_idx];

    int min_len = a_word->len < b_word->len ? a_word->len : b_word->len;
    int cmp = memcmp(a_word->str, b_word->str, min_len);
    if (cmp != 0)
    {
        return cmp;
    }

    return a_word->len - b_word->len;
}

void parse_text_words(char *str,
                      int str_len,
                      mem_word_t *words,
                      int n_words,
                      mem_word_t *unique_words,
                      int *n_unique_words_ptr)
{
    assert(str != NULL);
    assert(str_len > 0);
    assert(words != NULL);
    assert(n_words > 0);
    assert(unique_words != NULL);
    assert(n_unique_words_ptr != NULL);

    int str_idx = 0;
    char *cur_ptr = str;
    char *prev_ptr = cur_ptr;
    for (int i = 0; i < n_words; ++i)
    {
        words[i].str = cur_ptr;

        while (*cur_ptr != DELIM && *cur_ptr != '\0' && str_idx < str_len)
        {
            ++cur_ptr;
            ++str_idx;
        }

        words[i].len = cur_ptr - prev_ptr;
        prev_ptr = ++cur_ptr;
        ++str_idx;
    }

    int *idxs = malloc(sizeof(int) * n_words);
    assert(idxs != NULL);
    for (int i = 0; i < n_words; ++i)
    {
        idxs[i] = i;
    }

    g_sort_words = words;
    qsort(idxs, n_words, sizeof(int), word_index_cmp);

    char *canon = words[idxs[0]].str;
    unique_words[0] = words[idxs[0]];
    int unique_idx = 1;
    for (int i = 1; i < n_words; ++i)
    {
        mem_word_t *cur = &words[idxs[i]];
        mem_word_t *prev = &words[idxs[i - 1]];
        if (cur->len == prev->len && memcmp(cur->str, prev->str, cur->len) == 0)
        {
            cur->str = canon;
        }
        else
        {
            canon = cur->str;
            unique_words[unique_idx++] = *cur;
        }
    }

    *n_unique_words_ptr = unique_idx;

    free(idxs);

    for (int i = 0; i < n_words; ++i)
    {
        fprintf(out_stream, "Word [%d] \"", i);

        for (int j = 0; j < words[i].len; ++j)
        {
            fputc(words[i].str[j], out_stream);
        }

        fprintf(out_stream,
                "\" -> %p is %d characters long\n",
                (void *)words[i].str,
                words[i].len);
    }

    fprintf(out_stream, "Got %d unique words\n", *n_unique_words_ptr);

    for (int i = 0; i < *n_unique_words_ptr; ++i)
    {
        fprintf(out_stream, "Unique word [%d] \"", i);

        for (int j = 0; j < unique_words[i].len; ++j)
        {
            fputc(unique_words[i].str[j], out_stream);
        }

        fprintf(out_stream,
                "\" -> %p is %d characters long\n",
                (void *)unique_words[i].str,
                unique_words[i].len);
    }
}

typedef struct
{
    mem_word_t *fst;
    mem_word_t *sec;
} bigram_t;

int count_bigrams(int n_words)
{
    const int n_2grams = n_words - 1;

    fprintf(out_stream, "Generating %d 2-grams\n", n_2grams);

    return n_2grams;
}

void generate_bigrams(mem_word_t *words, bigram_t *bigrams, int n_bigrams)
{
    for (int i = 0; i < n_bigrams; ++i)
    {
        bigrams[i].fst = &words[i];
        bigrams[i].sec = &words[i + 1];
    }

    for (int i = 0; i < n_bigrams; ++i)
    {
        fprintf(out_stream, "2-Gram [%d] (\"", i);

        for (int j = 0; j < bigrams[i].fst->len; ++j)
        {
            fputc(bigrams[i].fst->str[j], out_stream);
        }

        fprintf(out_stream, "\" -> %p, \"", (void *)bigrams[i].fst);

        for (int j = 0; j < bigrams[i].sec->len; ++j)
        {
            fputc(bigrams[i].sec->str[j], out_stream);
        }

        fprintf(out_stream, "\" -> %p)\n", (void *)bigrams[i].sec);
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

int find_word_idx(ptr_idx_t *map, int n, char *ptr)
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
                          mem_word_t *unique_words,
                          int n_unique_words,
                          word_transitions_t *transitions)
{
    ptr_idx_t *map = malloc(sizeof(ptr_idx_t) * n_unique_words);
    assert(map != NULL);

    for (int i = 0; i < n_unique_words; ++i)
    {
        map[i].ptr = unique_words[i].str;
        map[i].idx = i;
    }

    qsort(map, n_unique_words, sizeof(ptr_idx_t), ptr_idx_cmp);

    for (int i = 0; i < n_unique_words; ++i)
    {
        transitions[i].items = NULL;
        transitions[i].len = 0;
        transitions[i].cap = 0;
        transitions[i].total_count = 0;
    }

    for (int i = 0; i < n_bigrams; ++i)
    {
        int src = find_word_idx(map, n_unique_words, bigrams[i].fst->str);
        int dst = find_word_idx(map, n_unique_words, bigrams[i].sec->str);
        assert(src != -1 && dst != -1);

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
                assert(wt->items != NULL);
            }

            wt->items[wt->len].next_uniq_idx = dst;
            wt->items[wt->len].count = 1;
            wt->len++;
        }

        ++wt->total_count;
    }

    free(map);

    for (int i = 0; i < n_unique_words; ++i)
    {
        word_transitions_t t = transitions[i];
        fprintf(out_stream,
                "Word [%d] has %d out of %d transitions\n",
                i,
                t.len,
                t.total_count);
    }
}

void generate_sentences(mem_word_t *unique_words,
                        int n_unique_words,
                        word_transitions_t *transitions)
{
    srand((unsigned)time(NULL));

    const int n_sentences = 32;
    const int n_words_per_sentence = n_unique_words < 0xFF ? n_unique_words
                                                           : 0xFF;

    fprintf(out_stream, "Generating %d sentences\n", n_sentences);

    for (int sentences = 0; sentences < n_sentences; ++sentences)
    {
        mem_word_t words[n_words_per_sentence];
        int words_len = 1;

        int word_idx = rand() % n_unique_words;
        words[0] = unique_words[word_idx];

        while (words_len < n_words_per_sentence)
        {
            word_transitions_t *wt = &transitions[word_idx];
            if (wt->len == 0)
            {
                word_idx = rand() % n_unique_words;
                words[words_len++] = unique_words[word_idx];
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
            words[words_len++] = unique_words[word_idx];
        }

        fputs("Sentence: \"", out_stream);
        for (int i = 0; i < words_len; ++i)
        {
            for (int j = 0; j < words[i].len; ++j)
            {
                fputc(words[i].str[j], out_stream);
            }

            if (i != words_len - 1)
            {
                fputc(' ', out_stream);
            }
        }

        fputs("\"\n", out_stream);
    }
}

int main(int argc, char **argv)
{
    char *corpus_path = NULL;
    char *out_path = NULL;
    for (int i = 1; i < argc; ++i)
    {
        char *s = argv[i];
        bool has_next = i + 1 < argc;

        if ((strcmp(s, "-c") == 0 || strcmp(s, "--corpus") == 0) && has_next)
        {
            corpus_path = argv[++i];
        }
        else if ((strcmp(s, "-l") == 0 || strcmp(s, "--log") == 0) && has_next)
        {
            out_path = argv[++i];
        }
    }

    open_corpus_stream(corpus_path);
    open_out_stream(out_path);

    int text_len = get_corpus_len();
    char *text = calloc(text_len, 1);
    assert(text != NULL);
    format_corpus_text(text, &text_len);

    fclose(corpus_stream);

    int n_words = count_words(text, text_len);
    mem_word_t *words = malloc(sizeof(*words) * n_words);
    assert(words != NULL);

    mem_word_t *unique_words = malloc(sizeof(*words) * n_words);
    assert(unique_words != NULL);
    int n_unique_words = 0;
    parse_text_words(text,
                     text_len,
                     words,
                     n_words,
                     unique_words,
                     &n_unique_words);

    int n_bigrams = count_bigrams(n_words);
    bigram_t *bigrams = malloc(sizeof(bigram_t) * n_bigrams);
    assert(bigrams != NULL);
    generate_bigrams(words, bigrams, n_bigrams);

    word_transitions_t *transitions = calloc(n_unique_words,
                                             sizeof(word_transitions_t));
    assert(transitions != NULL);
    build_adjacency_list(bigrams,
                         n_bigrams,
                         unique_words,
                         n_unique_words,
                         transitions);

    generate_sentences(unique_words, n_unique_words, transitions);

    free(unique_words);
    free(bigrams);
    free(words);
    free(text);

    fclose(out_stream);

    return 0;
}
