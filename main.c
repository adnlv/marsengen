#include <stdio.h>
#include <assert.h>

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

int main(int argc, char **argv)
{
    char *input_path = argc > 1 ? argv[1] : "input.txt";
    open_input_file(input_path);

    char *logs_path = argc > 2 ? argv[2] : "output.txt";
    open_log_file(logs_path);

    fclose(input_file);
    fclose(logs_file);

    return 0;
}