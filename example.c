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

void print_list(const char **items, size_t count)
{
    printf("[");
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) printf(", ");
        printf("%s", items[i]);
    }
    printf("]\n");
}

int main(int argc, char **argv)
{
    bool      *help    = flag_bool("help", false, "Print this help to stdout and exit with 0");
    bool      *Bool    = flag_bool("bool", false, "Boolean flag");
    float     *Float   = flag_float("float", 0.0f, "Float flag");
    double    *Double  = flag_double("double", 0.0f, "double flag");
    size_t    *size    = flag_size("size", 0, "Size flag");
    uint64_t  *integer = flag_uint64("integer", 0, "integer flag");
    char     **str     = flag_str("str", "", "String flag");
    Flag_List *list    = flag_list("list", "List flag");

    bool Bool2 = false;
    float Float2 = 0.0f;
    double Double2 = 0.0;
    size_t size2 = 0;
    uint64_t integer2 = 0;
    char *str2 = NULL;
    Flag_List list2 = {0};

    flag_bool_var(&Bool2, "bool2", false, "Boolean flag");
    flag_float_var(&Float2, "float2", false, "Float flag");
    flag_double_var(&Double2, "double2", false, "Double flag");
    flag_size_var(&size2, "size2", 0, "Size flag");
    flag_uint64_var(&integer2, "integer2", 0, "integer flag");
    flag_str_var(&str2, "str2", "", "String flag");
    flag_list_var(&list2, "list2", "List flag");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    argc = flag_rest_argc();
    argv = flag_rest_argv();

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
    print_list((const char **)argv, argc);
    return 0;
}
