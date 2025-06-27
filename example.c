#include <stdio.h>
#include <stdlib.h>

#define FLAG_IMPLEMENTATION
#include "./flag.h"

typedef struct {
    bool is_parse_error;
    Flag_Parse_Error parse_error;
} Example_Error;

Example_Error wrap_parse_error(Flag_Parse_Error error)
{
    return (Example_Error) {true, error};
}

void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./example [OPTIONS] [--] <OUTPUT FILES...>\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

[[noreturn]]
void panic(Example_Error problem)
{
    usage(stderr);
    if (problem.is_parse_error) {
        flag_print_error(stderr, problem.parse_error);
    } else {
        fprintf(stderr, "ERROR: no output files are provided\n");
    }
    exit(1);
}

int main(int argc, char **argv)
{
    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    char **line = flag_str("line", "Hi!", "Line to output to the file");
    size_t *count = flag_size("count", 64, "Amount of lines to generate");

    const Flag_Parse_Result parsed = flag_parse(argc, argv);

    if (RESULT_HAS_SUCCESS(parsed) && *help) {
        usage(stdout);
        exit(0);
    }

    RESULT_STRUCT(Flag_Parsed, Example_Error) result = RESULT_MAP_FAILURE(parsed, wrap_parse_error, typeof(result));
    #define VALIDATE_ARGC_LAMBDA(x) (x.rest_argc > 0)
    result = RESULT_FILTER(result, VALIDATE_ARGC_LAMBDA, (Example_Error) {false});

    RESULT_IF_FAILURE(result, panic);

    for (int i = 0; i < RESULT_USE_SUCCESS(result).rest_argc; ++i) {
        const char *file_path = RESULT_USE_SUCCESS(result).rest_argv[i];
        FILE *f = fopen(file_path, "w");
        assert(f);

        for (size_t i = 0; i < *count; ++i) {
            fprintf(f, "%s\n", *line);
        }

        fclose(f);

        printf("Generated %" PRIu64 " lines in %s\n", *count, file_path);
    }

    return 0;
}
