#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

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

    for (int i = 0; i < n_tokens; ++i)
    {
        fprintf(logs_file, "Token [%d] \"", i);
        for (int j = 0; j < tokens[i].len; ++j)
            fputc(tokens[i].ptr[j], logs_file);
        fprintf(logs_file, "\" is %d characters long\n", tokens[i].len);
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

    int tokens_len = count_tokens(text, text_len);
    token_t tokens[text_len];
    tokenize_input_text(text, text_len, tokens, tokens_len);

    fclose(input_file);
    fclose(logs_file);

    return 0;
}