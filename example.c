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
    bool *help = flag_bool("help", "Print this help to stdout and exit with 0");
    char **line = flag_str("line", "Line to output to the file");
    flag_default(line, "Hey");
    size_t *count = flag_size("count", "Amount of lines to generate");
    flag_default(count, 128);
    flag_required(count, true);
    float *test = flag_float("test", "A test float");

    bool argr = flag_parse(argc, argv);

    if (*help) {
        usage(stdout);
        exit(0);
    }

    if (!argr) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
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

        fclose(f);

        printf("Generated %" PRIu64 " lines in %s\n", *count, file_path);
    }

    printf("Test: %f\n", *test);

    return 0;
}
