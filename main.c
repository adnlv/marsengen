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

    /* Remove unsupported trailing characters */
    assert(fseek(input_file, 0, SEEK_SET) == 0);
    for (i = len - 1; i >= 0; --i)
    {
        char ch = buf[i];
        if (IS_LOWER(ch) || IS_UPPER(ch))
            break;

        buf[i] = '\0';
        --j;
    }

    *len_ptr = j;

    fprintf(logs_file, "Formatted text contains %d characters\n", j);
    fprintf(logs_file, "Formatted text: %s", buf);
}

int main(int argc, char **argv)
{
    char *input_path = argc > 1 ? argv[1] : "input.txt";
    open_input_file(input_path);

    char *logs_path = argc > 2 ? argv[2] : "output.txt";
    open_log_file(logs_path);

    int text_len = get_input_file_len();
    char text_buf[text_len];
    format_input_text(text_buf, &text_len);

    fclose(input_file);
    fclose(logs_file);

    return 0;
}