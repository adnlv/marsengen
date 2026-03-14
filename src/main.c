#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DELIM ' '
#define IS_LOWER(ch) ((ch) >= 'a' && (ch) <= 'z')
#define IS_UPPER(ch) ((ch) >= 'A' && (ch) <= 'Z')

static FILE *input_file = NULL;
static FILE *logs_file = NULL;

void open_input_file(const char *path)
{
    assert(path != NULL);

    input_file = fopen(path, "r");
    assert(input_file != NULL);

    printf("Reading from file \"%s\"\n", path);
}

void open_logs_file(const char *path)
{
    assert(path != NULL);

    logs_file = fopen(path, "w+");
    assert(logs_file != NULL);

    printf("Writing logs to file \"%s\"\n", path);
}

int get_input_file_len(void)
{
    assert(fseek(input_file, 0, SEEK_END) == 0);

    const int pos = ftell(input_file);

    assert(pos != -1);
    assert(fseek(input_file, 0, SEEK_SET) == 0);

    fprintf(logs_file, "Unformatted text contains %d characters\n", pos);

    return pos;
}

void format_input_text(char *buf, int *len_ptr)
{
    int len = 0;
    bool is_prev_letter = false;
    for (int i = 0; i < *len_ptr; ++i)
    {
        int c = getc(input_file);
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

    fprintf(logs_file, "Formatted text contains %d characters: %s\n", len, buf);
}

typedef struct
{
    char *ptr;
    int len;
} token_t;

int count_tokens(const char *str, int len)
{
    assert(str != NULL);
    assert(len > 0);

    int n_tokens = 1;
    for (int i = 0; i < len; ++i)
    {
        if (str[i] == DELIM)
        {
            ++n_tokens;
        }
    }

    fprintf(logs_file, "Text contains %d tokens\n", n_tokens);

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

void tokenize_input_text(char *str,
                         int str_len,
                         token_t *tokens,
                         int n_tokens,
                         token_t *uniques,
                         int *n_uniques_ptr)
{
    assert(str != NULL);
    assert(str_len > 0);
    assert(tokens != NULL);
    assert(n_tokens > 0);
    assert(uniques != NULL);
    assert(n_uniques_ptr != NULL);

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
    assert(idxs != NULL);
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
        fprintf(logs_file, "Token [%d] \"", i);

        for (int j = 0; j < tokens[i].len; ++j)
        {
            fputc(tokens[i].ptr[j], logs_file);
        }

        fprintf(logs_file,
                "\" -> %p is %d characters long\n",
                (void *)tokens[i].ptr,
                tokens[i].len);
    }

    fprintf(logs_file, "Got %d unique tokens\n", *n_uniques_ptr);

    for (int i = 0; i < *n_uniques_ptr; ++i)
    {
        fprintf(logs_file, "Unique token [%d] \"", i);

        for (int j = 0; j < uniques[i].len; ++j)
        {
            fputc(uniques[i].ptr[j], logs_file);
        }

        fprintf(logs_file,
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

    fprintf(logs_file, "Generating %d 2-grams\n", n_2grams);

    return n_2grams;
}

void generate_bigrams_from_tokens(token_t *tokens,
                                  bigram_t *bigrams,
                                  int n_bigrams)
{
    for (int i = 0; i < n_bigrams; ++i)
    {
        bigrams[i].fst = &tokens[i];
        bigrams[i].sec = &tokens[i + 1];
    }

    for (int i = 0; i < n_bigrams; ++i)
    {
        fprintf(logs_file, "2-Gram [%d] (\"", i);

        for (int j = 0; j < bigrams[i].fst->len; ++j)
        {
            fputc(bigrams[i].fst->ptr[j], logs_file);
        }

        fprintf(logs_file, "\" -> %p, \"", (void *)bigrams[i].fst);

        for (int j = 0; j < bigrams[i].sec->len; ++j)
        {
            fputc(bigrams[i].sec->ptr[j], logs_file);
        }

        fprintf(logs_file, "\" -> %p)\n", (void *)bigrams[i].sec);
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
    assert(map != NULL);

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

    for (int k = 0; k < n_bigrams; ++k)
    {
        int src = find_token_idx(map, n_uniques, bigrams[k].fst->ptr);
        int dst = find_token_idx(map, n_uniques, bigrams[k].sec->ptr);
        assert(src != -1 && dst != -1);

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

    for (int i = 0; i < n_uniques; ++i)
    {
        word_transitions_t t = transitions[i];
        fprintf(logs_file,
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

    fprintf(logs_file, "Generating %d sentences\n", n_sentences);

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

        fputs("Sentence: \"", logs_file);
        for (int i = 0; i < words_len; ++i)
        {
            for (int j = 0; j < words[i].len; ++j)
            {
                fputc(words[i].ptr[j], logs_file);
            }

            if (i != words_len - 1)
            {
                fputc(' ', logs_file);
            }
        }

        fputs("\"\n", logs_file);
    }
}

int main(void)
{
    open_input_file("input.txt");
    open_logs_file("output.txt");

    int text_len = get_input_file_len();
    char *text = calloc(text_len, 1);
    assert(text != NULL);
    format_input_text(text, &text_len);

    fclose(input_file);

    int n_tokens = count_tokens(text, text_len);
    token_t *tokens = malloc(sizeof(token_t) * n_tokens);
    assert(tokens != NULL);

    token_t *unique_tokens = malloc(sizeof(token_t) * n_tokens);
    assert(unique_tokens != NULL);
    int n_unique_tokens = 0;
    tokenize_input_text(text,
                        text_len,
                        tokens,
                        n_tokens,
                        unique_tokens,
                        &n_unique_tokens);

    int n_bigrams = count_bigrams(n_tokens);
    bigram_t *bigrams = malloc(sizeof(bigram_t) * n_bigrams);
    assert(bigrams != NULL);
    generate_bigrams_from_tokens(tokens, bigrams, n_bigrams);

    word_transitions_t *transitions = calloc(n_unique_tokens,
                                             sizeof(word_transitions_t));
    assert(transitions != NULL);
    build_adjacency_list(bigrams,
                         n_bigrams,
                         unique_tokens,
                         n_unique_tokens,
                         transitions);

    generate_sentences(unique_tokens, n_unique_tokens, transitions);

    free(unique_tokens);
    free(bigrams);
    free(tokens);
    free(text);

    fclose(logs_file);

    return 0;
}
