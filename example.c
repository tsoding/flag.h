#include <stdio.h>
#include <stdlib.h>

#define FLAG_IMPLEMENTATION
#include "./flag.h"

void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./example [OPTIONS] [--] [ARGS]\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

int main(int argc, char **argv)
{
    bool      *help   = flag_bool("help", false, "Print this help to stdout and exit with 0");
    bool      *Bool   = flag_bool("bool", false, "Boolean flag");
    size_t    *size   = flag_size("size", 0, "Size flag");
    uint64_t  *uint64 = flag_uint64("uint64", 0, "uint64 flag");
    char     **str    = flag_str("str", "", "String flag");
    Flag_List *list   = flag_list("list", "List flag");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help) {
        usage(stdout);
        exit(0);
    }

    // TODO: Would be nice to have some sort of mechanism to inspect all the defined flags
    //   Maybe just expose flag_global_context? But I'm afraid that this will make changing
    //   its internal structure in a backward compatible way more difficult...
    //   But the only reason we may want to change it right now is to make the array of the
    //   flags dynamic. Maybe we should just make it dynamic and finally expose the internal
    //   structure for good?
    int n, width = 0;
    n = strlen(flag_name(Bool));   if (n > width) width = n;
    n = strlen(flag_name(size));   if (n > width) width = n;
    n = strlen(flag_name(uint64)); if (n > width) width = n;
    n = strlen(flag_name(str));    if (n > width) width = n;
    n = strlen(flag_name(list));   if (n > width) width = n;

    printf("-%-*s => %s\n",          width, flag_name(Bool),   *Bool ? "true" : "false");
    printf("-%-*s => %zu\n",         width, flag_name(size),   *size);
    printf("-%-*s => %" PRIu64 "\n", width, flag_name(uint64), *uint64);
    printf("-%-*s => %s\n",          width, flag_name(str),    *str);
    printf("-%-*s => [",             width, flag_name(list));
    for (size_t i = 0; i < list->count; ++i) {
        if (i > 0) printf(", ");
        printf("%s", list->items[i]);
    }
    printf("]\n");
    return 0;
}
