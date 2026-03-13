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

int count_tokens(const char *str, const int len)
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

void tokenize_input_text(char *str, int str_len, token_t *tokens, int n_tokens)
{
    assert(str != NULL);
    assert(str_len > 0);
    assert(tokens != NULL);
    assert(n_tokens > 0);

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
        }
    }

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
}

typedef struct
{
    token_t *fst;
    token_t *sec;
} bigram_t;

int count_bigrams(const int n_tokens)
{
    const int n_2grams = n_tokens - 1;

    fprintf(logs_file, "Generating %d 2-grams\n", n_2grams);

    return n_2grams;
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

void get_unique_tokens(token_t *toks,
                       const int n_toks,
                       token_t *uniqs,
                       int *n_uniqs_ptr)
{
    int uniq_idx = 0;
    for (int i = 0; i < n_toks; ++i)
    {
        token_t *dup = NULL;
        for (int j = 0; j < i; ++j)
        {
            if (toks[i].ptr == toks[j].ptr)
            {
                dup = &toks[j];
                break;
            }
        }

        if (dup == NULL)
        {
            uniqs[uniq_idx] = toks[i];
            ++uniq_idx;
        }
    }

    int n_uniqs = uniq_idx;
    fprintf(logs_file, "Got %d unique tokens\n", n_uniqs);

    for (int i = 0; i < n_uniqs; ++i)
    {
        fprintf(logs_file, "Unique token [%d] \"", i);

        for (int j = 0; j < uniqs[i].len; ++j)
        {
            fputc(uniqs[i].ptr[j], logs_file);
        }

        fprintf(logs_file,
                "\" -> %p is %d characters long\n",
                (void *)uniqs[i].ptr,
                uniqs[i].len);
    }

    *n_uniqs_ptr = n_uniqs;
}

void fill_transition_matrix(bigram_t *bigrams,
                            const int n_bigrams,
                            token_t *uniqs,
                            const int n_uniqs,
                            double *mat)
{
    for (int i = 0; i < n_uniqs; ++i)
    {
        char *fst = uniqs[i].ptr;
        for (int j = 0; j < n_uniqs; ++j)
        {
            char *sec = uniqs[j].ptr;
            int count = 0;
            for (int k = 0; k < n_bigrams; ++k)
            {
                if (bigrams[k].fst->ptr == fst && bigrams[k].sec->ptr == sec)
                {
                    ++count;
                }
            }

            mat[j * n_uniqs + i] = (double)count;
        }

        double sum = 0;
        for (int j = 0; j < n_uniqs; ++j)
        {
            sum += mat[j * n_uniqs + i];
        }

        if (sum != 0)
        {
            for (int j = 0; j < n_uniqs; ++j)
            {
                mat[j * n_uniqs + i] /= sum;
            }
        }
    }

    fprintf(logs_file, "Transition matrix [%d by %d]\n", n_uniqs, n_uniqs);
    for (int i = 0; i < n_uniqs; ++i)
    {
        fputc('[', logs_file);
        for (int j = 0; j < n_uniqs; ++j)
        {
            fprintf(logs_file, "%.2lf", mat[j * n_uniqs + i]);
            if (j != n_uniqs - 1)
            {
                fputc(',', logs_file);
            }
        }
        fputs("]\n", logs_file);
    }
}

void generate(token_t *uniqs, const int n_uniqs, double *mat)
{
    srand((unsigned)time(NULL));

    const int n_sentences = 32;
    const int n_words_per_sentence = n_uniqs < 0xFF ? n_uniqs : 0xFF;

    fprintf(logs_file, "Generating %d sentences\n", n_sentences);

    for (int sentences = 0; sentences < n_sentences; ++sentences)
    {
        token_t words[n_words_per_sentence];
        int words_len = 1;

        int word_idx = rand() % n_uniqs;
        words[0] = uniqs[word_idx];

        while (words_len < n_words_per_sentence)
        {
            double sum = 0.0;
            for (int i = 0; i < n_uniqs; ++i)
            {
                sum += mat[i * n_uniqs + word_idx];
            }

            if (sum <= 0.0)
            {
                word_idx = rand() % n_uniqs;
                words[words_len++] = uniqs[word_idx];
                continue;
            }

            double r = ((double)rand() / ((double)RAND_MAX + 1.0)) * sum;
            double cumulative = 0.0;
            int chosen = -1;
            for (int i = 0; i < n_uniqs; ++i)
            {
                cumulative += mat[i * n_uniqs + word_idx];
                if (r < cumulative)
                {
                    chosen = i;
                    break;
                }
            }

            if (chosen == -1)
            {
                for (int i = n_uniqs - 1; i >= 0; --i)
                {
                    if (mat[i * n_uniqs + word_idx] > 0.0)
                    {
                        chosen = i;
                        break;
                    }
                }

                if (chosen == -1)
                {
                    chosen = rand() % n_uniqs;
                }
            }

            word_idx = chosen;
            words[words_len++] = uniqs[word_idx];
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
    tokenize_input_text(text, text_len, tokens, n_tokens);

    token_t *uniq_toks = malloc(sizeof(token_t) * n_tokens);
    assert(uniq_toks != NULL);
    int n_uniqs = 0;
    get_unique_tokens(tokens, n_tokens, uniq_toks, &n_uniqs);

    int n_bigrams = count_bigrams(n_tokens);
    bigram_t *bigrams = malloc(sizeof(bigram_t) * n_bigrams);
    assert(bigrams != NULL);
    generate_bigrams_from_tokens(tokens, bigrams, n_bigrams);

    double *trans_mat = calloc(n_uniqs * n_uniqs, sizeof(double));
    assert(trans_mat != NULL);
    fill_transition_matrix(bigrams, n_bigrams, uniq_toks, n_uniqs, trans_mat);

    generate(uniq_toks, n_uniqs, trans_mat);

    free(trans_mat);
    free(uniq_toks);
    free(bigrams);
    free(tokens);
    free(text);

    fclose(logs_file);

    return 0;
}
