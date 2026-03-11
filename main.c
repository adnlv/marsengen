#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

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

void open_log_file(const char *path)
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
    int i = 0;
    int j = 0;
    const int len = *len_ptr;
    for (; i < len; ++i)
        buf[i] = '\0';

    bool is_prev_letter = false;
    for (i = 0; i < len; ++i)
    {
        char ch = getc(input_file);
        if (ch <= 0)
            break;

        bool is_lower = IS_LOWER(ch);
        bool is_upper = IS_UPPER(ch);
        if (is_lower || is_upper)
        {
            if (is_upper)
                ch += 32;

            buf[j] = ch;
            is_prev_letter = true;
            ++j;
        }
        else if (is_prev_letter)
        {
            is_prev_letter = false;
            buf[j++] = DELIM;
        }
    }

    buf[--j] = '\0';
    *len_ptr = j;

    fprintf(logs_file, "Formatted text contains %d characters\n", j);
    fprintf(logs_file, "Formatted text: %s\n", buf);
}

typedef struct token
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
            ++n_tokens;
    }

    fprintf(logs_file, "Text contains %d tokens\n", n_tokens);

    return n_tokens;
}

void tokenize_input_text(char *str,
                         const int str_len,
                         token_t *tokens,
                         const int n_tokens)
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

        while (*cur_ptr != DELIM && str_idx < str_len)
        {
            ++cur_ptr;
            ++str_idx;
        }

        tokens[i].len = cur_ptr - prev_ptr;
        prev_ptr = ++cur_ptr;
        ++str_idx;
    }

    /**
     * Optimize string comparisons by pointing to the first word occurrence.
     * This allows to compare tokens by their strings memory addresses.
     */
    for (int i = 0; i < n_tokens; ++i)
    {
        token_t *t1 = &tokens[i];
        for (int j = 0; j < n_tokens; ++j)
        {
            token_t *t2 = &tokens[j];

            if (t1->len != t2->len)
                continue;

            if (strncmp(t1->ptr, t2->ptr, t1->len) != 0)
                continue;

            t2->ptr = t1->ptr;
        }
    }

    for (int i = 0; i < n_tokens; ++i)
    {
        fprintf(logs_file, "Token [%d] \"", i);

        for (int j = 0; j < tokens[i].len; ++j)
            fputc(tokens[i].ptr[j], logs_file);

        fprintf(logs_file,
                "\" -> %p is %d characters long\n",
                (void *)tokens[i].ptr,
                tokens[i].len);
    }
}

typedef struct ngram2
{
    token_t *fst;
    token_t *sec;
} ngram2_t;

int count_2grams(const int n_tokens)
{
    const int n_2grams = n_tokens - 1;

    fprintf(logs_file, "Generating %d 2-grams\n", n_2grams);

    return n_2grams;
}

void gen_2grams_from_tokens(token_t *tokens,
                            const int n_tokens,
                            ngram2_t *ngram2s,
                            const int n_2grams)
{
    for (int i = 0; i < n_2grams; ++i)
    {
        ngram2s[i].fst = &tokens[i];
        ngram2s[i].sec = &tokens[i + 1];
    }

    for (int i = 0; i < n_2grams; ++i)
    {
        fprintf(logs_file, "2-Gram [%d] (\"", i);

        for (int j = 0; j < ngram2s[i].fst->len; ++j)
            fputc(ngram2s[i].fst->ptr[j], logs_file);

        fprintf(logs_file, "\" -> %p, \"", (void *)ngram2s[i].fst);

        for (int j = 0; j < ngram2s[i].sec->len; ++j)
            fputc(ngram2s[i].sec->ptr[j], logs_file);

        fprintf(logs_file, "\" -> %p)\n", (void *)ngram2s[i].sec);
    }
}

int main(int argc, char **argv)
{
    char *input_path = argc > 1 ? argv[1] : "input.txt";
    open_input_file(input_path);

    char *logs_path = argc > 2 ? argv[2] : "output.txt";
    open_log_file(logs_path);

    int text_len = get_input_file_len();
    char text[text_len];
    format_input_text(text, &text_len);

    int n_tokens = count_tokens(text, text_len);
    token_t tokens[n_tokens];
    tokenize_input_text(text, text_len, tokens, n_tokens);

    int n_2grams = count_2grams(n_tokens);
    ngram2_t ngram2s[n_2grams];
    gen_2grams_from_tokens(tokens, n_tokens, ngram2s, n_2grams);

    fclose(input_file);
    fclose(logs_file);

    return 0;
}