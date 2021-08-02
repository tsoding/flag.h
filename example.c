#include <stdio.h>
#include <stdlib.h>

#define FLAG_IMPLEMENTATION
#include "./flag.h"

void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./example [OPTIONS]\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

int main(int argc, char **argv)
{
    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    char **output = flag_str("output", "output.txt", "Output file path");
    char **line = flag_str("line", "Hi!", "Line to output to the file");
    uint64_t *count = flag_uint64("count", 64, "Amount of lines to generate");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help) {
        usage(stdout);
        exit(0);
    }

    FILE *f = fopen(*output, "w");
    assert(f);

    for (uint64_t i = 0; i < *count; ++i) {
        fprintf(f, "%s\n", *line);
    }

    fclose(f);

    printf("Generated %" PRIu64 " lines in %s\n", *count, *output);

    return 0;
}
