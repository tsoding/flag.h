#include <stdlib.h>
#include <stdio.h>

#define FLAG_IMPLEMENTATION
#include "./flag.h"

void usage(FILE *const stream, const char *const program_name)
{
    fprintf(stream, "Usage: %s [OPTIONS] [--] [ARGS]\n", program_name);
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

void print_list(const char **items, size_t count)
{
    printf("[");
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) printf(", ");
        printf("%s", items[i]);
    }
    printf("]\n");
}

int main(int argc, const char **argv)
{
    bool        *help    = flag_bool("help", false, "Print this help to stdout and exit with 0");
    bool        *Bool    = flag_bool("bool", false, "Boolean flag");
    float       *Float   = flag_float("float", 0.0f, "Float flag");
    double      *Double  = flag_double("double", 0.0, "double flag");
    size_t      *size    = flag_size("size", 0, "Size flag");
    uint64_t    *integer = flag_uint64("integer", 0, "integer flag");
    const char **str     = flag_str("str", "", "String flag");
    Flag_List   *list    = flag_list("list", "List flag");

    bool        Bool2;
    float       Float2;
    double      Double2;
    size_t      size2;
    uint64_t    integer2;
    const char *str2;
    Flag_List   list2 = {NULL, 0, 0};

    flag_bool_var(&Bool2, "bool2", false, "Boolean flag");
    flag_float_var(&Float2, "float2", 0.0f, "Float flag");
    flag_double_var(&Double2, "double2", 0.0, "Double flag");
    flag_size_var(&size2, "size2", 0, "Size flag");
    flag_uint64_var(&integer2, "integer2", 0, "integer flag");
    flag_str_var(&str2, "str2", "", "String flag");
    flag_list_var(&list2, "list2", "List flag");

    if (!flag_parse(argc, argv)) {
        usage(stderr, flag_program_name());
        flag_print_error(stderr);
        exit(EXIT_FAILURE);
    }

    argc = flag_rest_argc();
    argv = flag_rest_argv();

    if (*help) {
        usage(stderr, flag_program_name());
        exit(EXIT_SUCCESS);
    }

    // TODO: Would be nice to have some sort of mechanism to inspect all the defined flags
    //   Maybe just expose flag_global_context? But I'm afraid that this will make changing
    //   its internal structure in a backward compatible way more difficult...
    //   But the only reason we may want to change it right now is to make the array of the
    //   flags dynamic. Maybe we should just make it dynamic and finally expose the internal
    //   structure for good?
    int n, width = 0;
    n = strlen(flag_name(Bool));      if (n > width) width = n;
    n = strlen(flag_name(Float));     if (n > width) width = n;
    n = strlen(flag_name(Double));    if (n > width) width = n;
    n = strlen(flag_name(size));      if (n > width) width = n;
    n = strlen(flag_name(integer));   if (n > width) width = n;
    n = strlen(flag_name(str));       if (n > width) width = n;
    n = strlen(flag_name(list));      if (n > width) width = n;
    n = strlen(flag_name(&Bool2));    if (n > width) width = n;
    n = strlen(flag_name(&Float2));   if (n > width) width = n;
    n = strlen(flag_name(&Double2));  if (n > width) width = n;
    n = strlen(flag_name(&size2));    if (n > width) width = n;
    n = strlen(flag_name(&integer2)); if (n > width) width = n;
    n = strlen(flag_name(&str2));     if (n > width) width = n;
    n = strlen(flag_name(&list2));    if (n > width) width = n;
    n = strlen("args");               if (n > width) width = n;

    printf("-%-*s => %s\n",          width, flag_name(Bool),      *Bool ? "true" : "false");
    printf("-%-*s => %f\n",          width, flag_name(Float),     *Float);
    printf("-%-*s => %lf\n",         width, flag_name(Double),    *Double);
    printf("-%-*s => %zu\n",         width, flag_name(size),      *size);
    printf("-%-*s => %" PRIu64 "\n", width, flag_name(integer),   *integer);
    printf("-%-*s => %s\n",          width, flag_name(str),       *str);
    printf("-%-*s => ",              width, flag_name(list));
    print_list(list->items, list->count);
    printf("-%-*s => %s\n",          width, flag_name(&Bool2),    Bool2 ? "true" : "false");
    printf("-%-*s => %f\n",          width, flag_name(&Float2),   Float2);
    printf("-%-*s => %lf\n",         width, flag_name(&Double2),  Double2);
    printf("-%-*s => %zu\n",         width, flag_name(&size2),    size2);
    printf("-%-*s => %" PRIu64 "\n", width, flag_name(&integer2), integer2);
    printf("-%-*s => %s\n",          width, flag_name(&str2),     str2);
    printf("-%-*s => ",              width, flag_name(&list2));
    print_list(list2.items, list2.count);

    printf("%-*s  => ", width, "args");
    print_list(argv, argc);
    return EXIT_SUCCESS;
}
