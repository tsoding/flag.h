#include <stdio.h>
#include <stdlib.h>

#define FLAG_IMPLEMENTATION
#include "./flag.h"

void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./example [OPTIONS] [--] <OUTPUT FILES...>\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

int main(int argc, char **argv)
{
    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    char **line = flag_str("line", "Hi!", "Line to output to the file");
    size_t *count = flag_size("count", 64, "Amount of lines to generate");
    Flag_List *extra = flag_list("L", "Extra lines to append to the end");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help) {
        usage(stdout);
        exit(0);
    }

    int rest_argc = flag_rest_argc();
    char **rest_argv = flag_rest_argv();

    if (rest_argc <= 0) {
        usage(stderr);
        fprintf(stderr, "ERROR: no output files are provided\n");
        exit(1);
    }

    for (int i = 0; i < rest_argc; ++i) {
        const char *file_path = rest_argv[i];
        FILE *f = fopen(file_path, "w");
        assert(f);

        for (size_t i = 0; i < *count; ++i) {
            fprintf(f, "%s\n", *line);
        }
        for (size_t i = 0; i < extra->count; ++i) {
            fprintf(f, "%s\n", extra->items[i]);
        }

        fclose(f);

        printf("Generated %" PRIu64 " lines in %s\n", *count + extra->count, file_path);
    }

    return 0;
}
