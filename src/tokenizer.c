#include "tokenizer.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define IS_LOWER(ch) ((ch) >= 'a' && (ch) <= 'z')
#define IS_UPPER(ch) ((ch) >= 'A' && (ch) <= 'Z')

FILE *open_corpus_file(const char *path)
{
    if (path == NULL)
    {
        fprintf(stderr, "Error: Corpus file path is NULL\n");
        return NULL;
    }

    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        fprintf(stderr, "Error: Failed to open corpus file \"%s\": %s\n", path, strerror(errno));
        return NULL;
    }

    return f;
}

long get_corpus_file_len(FILE *corpus_file)
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

void format_corpus_text(FILE *corpus_file, char *buf, int *len_ptr)
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

static int token_ptr_cmp(const void *a, const void *b)
{
    const token_t *tok_a = *(const token_t * const *)a;
    const token_t *tok_b = *(const token_t * const *)b;

    int min_len = tok_a->len < tok_b->len ? tok_a->len : tok_b->len;
    int cmp = memcmp(tok_a->ptr, tok_b->ptr, (size_t)min_len);
    if (cmp != 0)
    {
        return cmp;
    }

    return tok_a->len - tok_b->len;
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

    token_t **ptrs = malloc(sizeof(token_t *) * (size_t)n_tokens);
    if (ptrs == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for token sorting pointers\n");
        return false;
    }

    for (int i = 0; i < n_tokens; ++i)
    {
        ptrs[i] = &tokens[i];
    }

    qsort(ptrs, (size_t)n_tokens, sizeof(token_t *), token_ptr_cmp);

    char *canon = ptrs[0]->ptr;
    uniques[0] = *ptrs[0];
    int unique_idx = 1;
    for (int i = 1; i < n_tokens; ++i)
    {
        token_t *cur = ptrs[i];
        token_t *prev = ptrs[i - 1];
        if (cur->len == prev->len && memcmp(cur->ptr, prev->ptr, (size_t)cur->len) == 0)
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

    free(ptrs);

    return true;
}
