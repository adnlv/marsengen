#include "io.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static int mkdir_if_not_exists(const char *path)
{
    int result = mkdir(path, 0755);
    if (result == -1 && errno != EEXIST)
    {
        return result;
    }

    return 0;
}

FILE *fopen_output(const char *path)
{
    char *buf = NULL;
    if (path == NULL)
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
        if (buf == NULL)
        {
            return NULL;
        }

        mkdir_if_not_exists(dir);
        snprintf(buf, len, "%s/%lu.log", dir, (unsigned long)secs);
    }

    FILE *fd = fopen(buf, "w+");
    if (fd == NULL)
    {
        return NULL;
    }

    if (path == NULL)
    {
        free(buf);
    }

    return fd;
}

FILE *fopen_corpus(const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }

    return fopen(path, "r");
}
