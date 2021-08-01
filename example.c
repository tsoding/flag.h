#include <stdio.h>
#include <stdlib.h>

#define FLAG_IMPLEMENTATION
#include "./flag.h"

int main(int argc, char **argv)
{
    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    char **output = flag_str("output", "output.txt", "Output file path");
    char **line = flag_str("line", "Hi!", "Line to output to the file");
    uint64_t *count = flag_uint64("count", 64, "Amount of lines to generate");
    flag_uint64_range(count, 0, 1024);

    flag_parse(argc, argv);

    if (*help) {
        assert(argc >= 1);
        fprintf(stdout, "Usage: %s [OPTIONS]\n", argv[0]);
        flag_print_help(stdout);
        exit(1);
    }

    FILE *f = fopen(*output, "w");
    assert(f);

    for (uint64_t i = 0; i < *count; ++i) {
        fprintf(f, "%s\n", *line);
    }

    fclose(f);

    printf("Generated %"PRIu64" lines in %s\n", *count, *output);

    return 0;
}
