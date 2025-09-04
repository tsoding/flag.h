// flag.h -- v1.4.1 -- command-line flag parsing
//
//   Inspired by Go's flag module: https://pkg.go.dev/flag
//
// Macros API:
// - FLAG_LIST_INIT_CAP - initial capacity of the Flag_List and Flag_List_Mut Dynamic Arrays.
// - FLAGS_CAP - how many flags you can define within a single context.
// - FLAG_PUSH_DASH_DASH_BACK - make flag_parse() retain "--" in the rest args
//   (available via flag_rest_argc() and flag_rest_argv()). Useful when you need
//   to know whether flag_parse() has stopped due to encountering "--" or due to
//   encountering a non-flag. Ideally this should've been a default behavior,
//   but it breaks backward compatibility. Hence it's a feature macro.
//   TODO: make FLAG_PUSH_DASH_DASH_BACK a default behavior on a major version upgrade.
#ifndef FLAG_H_
#define FLAG_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

// TODO: *_var function variants
// void flag_bool_var(bool *var, const char *name, bool def, const char *desc);
// void flag_bool_uint64(uint64_t *var, const char *name, bool def, const char *desc);
// etc.
// WARNING! *_var functions may break the flag_name() functionality

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif // FLAGS_CAP

#ifndef FLAG_LIST_INIT_CAP
#define FLAG_LIST_INIT_CAP 8
#endif // FLAG_LIST_INIT_CAP

// Works with both Flag_List and Flag_List_Mut
#define flag_list_append(type, list, item)                                                         \
    do {                                                                                           \
        if ((list)->count >= (list)->capacity) {                                                   \
            size_t new_capacity = (list)->capacity == 0 ? FLAG_LIST_INIT_CAP : (list)->capacity*2; \
            (list)->items = (type*)realloc((list)->items, new_capacity*sizeof(*(list)->items));    \
            (list)->capacity = new_capacity;                                                       \
        }                                                                                          \
                                                                                                   \
        (list)->items[(list)->count++] = item;                                                     \
    } while(0)

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} Flag_List;

// The only reason Flag_List_Mut exists is to enable recursive usage of flag[_c]_parse(..) in a backward compatible manner.
// That is using flag[_c]_parse(..) on Flag_List_Mut-s acquired from other flag[_c]_parse(..) calls, since argv must be mutable
// to enable -flag=value syntax.
// TODO: It was a mistake to make items const in Flag_List in the first place. In the next major release get rid of Flag_List_Mut and make items mutable in Flag_List
typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} Flag_List_Mut;

/// API that operate on implicit global flag context

char *flag_name(void *val);
bool *flag_bool(const char *name, bool def, const char *desc);
uint64_t *flag_uint64(const char *name, uint64_t def, const char *desc);
size_t *flag_size(const char *name, uint64_t def, const char *desc);
char **flag_str(const char *name, const char *def, const char *desc);
Flag_List *flag_list(const char *name, const char *desc);
Flag_List_Mut *flag_list_mut(const char *name, const char *desc);
bool flag_parse(int argc, char **argv);
int flag_rest_argc(void);
char **flag_rest_argv(void);
const char *flag_program_name(void);
void flag_print_error(FILE *stream);
void flag_print_options(FILE *stream);

/// API that operate on a custom opaque flag context.

// Allocate a new opaque flag context.
//
// Setting `program_name` is needed so `flag_c_parse(c, argc, argv)` does not try to consume
// extra argument from `argv` as it usually does. If that's exactly what you want just pass
// `NULL` as the `program_name` then.
//
// If specific `program_name` does not make sense in your case, you can just set it to whatever,
// since internally it's not used for anything except consuming an extra argument at the beginning
// of flag[_c]_parse(..) and giving that argument back to the user via flag[_c]_program_name(..).
// If you are designing a subcommand system where each subcommand has its own context you can set
// `program_name` to the name of the corresponding subcommand.
void *flag_c_new(const char *program_name);
void flag_c_free(void *c);
char *flag_c_name(void *c, void *val);
bool *flag_c_bool(void *c, const char *name, bool def, const char *desc);
uint64_t *flag_c_uint64(void *c, const char *name, uint64_t def, const char *desc);
size_t *flag_c_size(void *c, const char *name, uint64_t def, const char *desc);
char **flag_c_str(void *c, const char *name, const char *def, const char *desc);
Flag_List *flag_c_list(void *c, const char *name, const char *desc);
Flag_List_Mut *flag_c_list_mut(void *c, const char *name, const char *desc);
bool flag_c_parse(void *c, int argc, char **argv);
int flag_c_rest_argc(void *c);
char **flag_c_rest_argv(void *c);
const char *flag_c_program_name(void *c);
void flag_c_print_error(void *c, FILE *stream);
void flag_c_print_options(void *c, FILE *stream);

#endif // FLAG_H_

//////////////////////////////

#ifdef FLAG_IMPLEMENTATION

typedef enum {
    FLAG_BOOL = 0,
    FLAG_UINT64,
    FLAG_SIZE,
    FLAG_STR,
    FLAG_LIST,
    FLAG_LIST_MUT,
    COUNT_FLAG_TYPES,
} Flag_Type;

static_assert(COUNT_FLAG_TYPES == 6, "Exhaustive Flag_Value definition");
typedef union {
    char *as_str;
    uint64_t as_uint64;
    bool as_bool;
    size_t as_size;
    Flag_List as_list;
    Flag_List_Mut as_list_mut;
} Flag_Value;

typedef enum {
    FLAG_NO_ERROR = 0,
    FLAG_ERROR_UNKNOWN,
    FLAG_ERROR_NO_VALUE,
    FLAG_ERROR_INVALID_NUMBER,
    FLAG_ERROR_INTEGER_OVERFLOW,
    FLAG_ERROR_INVALID_SIZE_SUFFIX,
    COUNT_FLAG_ERRORS,
} Flag_Error;

typedef struct {
    Flag_Type type;
    char *name;
    char *desc;
    Flag_Value val;
    Flag_Value def;
} Flag;

typedef struct {
    Flag flags[FLAGS_CAP];
    size_t flags_count;

    Flag_Error flag_error;
    char *flag_error_name;

    const char *program_name;

    int rest_argc;
    char **rest_argv;
} Flag_Context;

static Flag *flag__new_flag(Flag_Context *c, Flag_Type type, const char *name, const char *desc);

static Flag_Context flag_global_context;

void *flag_c_new(const char *program_name)
{
    Flag_Context *fc = (Flag_Context *)malloc(sizeof(*fc));
    memset(fc, 0, sizeof(*fc));
    fc->program_name = program_name;
    return fc;
}

void flag_c_free(void *c)
{
    free(c);
}

static Flag *flag__new_flag(Flag_Context *c, Flag_Type type, const char *name, const char *desc)
{
    assert(c->flags_count < FLAGS_CAP);
    Flag *flag = &c->flags[c->flags_count++];
    memset(flag, 0, sizeof(*flag));
    flag->type = type;
    // NOTE: I won't touch them I promise Kappa
    flag->name = (char*) name;
    flag->desc = (char*) desc;
    return flag;
}

char *flag_name(void *val)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    return flag->name;
}

char *flag_c_name(void *c, void *val)
{
    (void)c;
    // NOTE: right now internally there is no difference between flag_name(..) and flag_c_name(..)
    // But we are separating them on the API level in case something changes. Especially if we are
    // to implement the *_var function variants.
    return flag_name(val);
}

bool *flag_c_bool(void *c, const char *name, bool def, const char *desc)
{
    Flag *flag = flag__new_flag((Flag_Context*)c, FLAG_BOOL, name, desc);
    flag->def.as_bool = def;
    flag->val.as_bool = def;
    return &flag->val.as_bool;
}

bool *flag_bool(const char *name, bool def, const char *desc)
{
    return flag_c_bool(&flag_global_context, name, def, desc);
}

uint64_t *flag_c_uint64(void *c, const char *name, uint64_t def, const char *desc)
{
    Flag *flag = flag__new_flag((Flag_Context*)c, FLAG_UINT64, name, desc);
    flag->val.as_uint64 = def;
    flag->def.as_uint64 = def;
    return &flag->val.as_uint64;
}

uint64_t *flag_uint64(const char *name, uint64_t def, const char *desc)
{
    return flag_c_uint64(&flag_global_context, name, def, desc);
}

size_t *flag_c_size(void *c, const char *name, uint64_t def, const char *desc)
{
    Flag *flag = flag__new_flag((Flag_Context*)c, FLAG_SIZE, name, desc);
    flag->val.as_size = def;
    flag->def.as_size = def;
    return &flag->val.as_size;
}

size_t *flag_size(const char *name, uint64_t def, const char *desc)
{
    return flag_c_size(&flag_global_context, name, def, desc);
}

char **flag_c_str(void *c, const char *name, const char *def, const char *desc)
{
    Flag *flag = flag__new_flag((Flag_Context*)c, FLAG_STR, name, desc);
    flag->val.as_str = (char*) def;
    flag->def.as_str = (char*) def;
    return &flag->val.as_str;
}

char **flag_str(const char *name, const char *def, const char *desc)
{
    return flag_c_str(&flag_global_context, name, def, desc);
}

Flag_List *flag_c_list(void *c, const char *name, const char *desc)
{
    Flag *flag = flag__new_flag((Flag_Context*)c, FLAG_LIST, name, desc);
    return &flag->val.as_list;
}

Flag_List_Mut *flag_c_list_mut(void *c, const char *name, const char *desc)
{
    Flag *flag = flag__new_flag((Flag_Context*)c, FLAG_LIST_MUT, name, desc);
    return &flag->val.as_list_mut;
}

Flag_List *flag_list(const char *name, const char *desc)
{
    return flag_c_list(&flag_global_context, name, desc);
}

Flag_List_Mut *flag_list_mut(const char *name, const char *desc)
{
    return flag_c_list_mut(&flag_global_context, name, desc);
}

static char *flag_shift_args(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

int flag_rest_argc(void)
{
    return flag_global_context.rest_argc;
}

int flag_c_rest_argc(void *c)
{
    return ((Flag_Context*)c)->rest_argc;
}

char **flag_rest_argv(void)
{
    return flag_global_context.rest_argv;
}

char **flag_c_rest_argv(void *c)
{
    return ((Flag_Context*)c)->rest_argv;
}

const char *flag_program_name(void)
{
    return flag_global_context.program_name;
}

const char *flag_c_program_name(void *c)
{
    return ((Flag_Context*)c)->program_name;
}

void flag_c_set_program_name(void *c, const char *program_name)
{
    ((Flag_Context *)c)->program_name = program_name;
}

bool flag_c_parse(void *c, int argc, char **argv)
{
    Flag_Context *fc = (Flag_Context*)c;

    if (fc->program_name == NULL) {
        fc->program_name = flag_shift_args(&argc, &argv);
    }

    while (argc > 0) {
        char *flag = flag_shift_args(&argc, &argv);

        if (*flag != '-') {
            // NOTE: pushing flag back into args
            fc->rest_argc = argc + 1;
            fc->rest_argv = argv - 1;
            return true;
        }

        if (strcmp(flag, "--") == 0) {
#ifdef FLAG_PUSH_DASH_DASH_BACK
            // NOTE: pushing dash dash back into args as requested by the user
            fc->rest_argc = argc + 1;
            fc->rest_argv = argv - 1;
#else
            // NOTE: not pushing dash dash back into args for backward compatibility
            fc->rest_argc = argc;
            fc->rest_argv = argv;
#endif // FLAG_PUSH_DASH_DASH_BACK
            return true;
        }

        // NOTE: remove the dash
        flag += 1;

        char *equals = strchr(flag, '=');
        if (equals != NULL) {
            // trim off the '=' and the value from `flag`,
            *equals = '\0';
            // and make `equals` be a pointer to just the value
            equals += 1;
        }

        bool found = false;
        for (size_t i = 0; i < fc->flags_count; ++i) {
            if (strcmp(fc->flags[i].name, flag) == 0) {
                static_assert(COUNT_FLAG_TYPES == 6, "Exhaustive flag type parsing");
                switch (fc->flags[i].type) {
                case FLAG_LIST: {
                    char *arg;
                    if (equals == NULL) {
                        if (argc == 0) {
                            fc->flag_error = FLAG_ERROR_NO_VALUE;
                            fc->flag_error_name = flag;
                            return false;
                        }
                        arg = flag_shift_args(&argc, &argv);
                    } else {
                        arg = equals;
                    }

                    flag_list_append(const char *, &fc->flags[i].val.as_list, arg);
                }
                break;

                case FLAG_LIST_MUT: {
                    char *arg;
                    if (equals == NULL) {
                        if (argc == 0) {
                            fc->flag_error = FLAG_ERROR_NO_VALUE;
                            fc->flag_error_name = flag;
                            return false;
                        }
                        arg = flag_shift_args(&argc, &argv);
                    } else {
                        arg = equals;
                    }

                    flag_list_append(char *, &fc->flags[i].val.as_list_mut, arg);
                }
                break;

                case FLAG_BOOL: {
                    fc->flags[i].val.as_bool = true;
                }
                break;

                case FLAG_STR: {
                    char *arg;
                    if (equals == NULL) {
                        if (argc == 0) {
                            fc->flag_error = FLAG_ERROR_NO_VALUE;
                            fc->flag_error_name = flag;
                            return false;
                        }
                        arg = flag_shift_args(&argc, &argv);
                    } else {
                        arg = equals;
                    }

                    fc->flags[i].val.as_str = arg;
                }
                break;

                case FLAG_UINT64: {
                    char *arg;
                    if (equals == NULL) {
                        if (argc == 0) {
                            fc->flag_error = FLAG_ERROR_NO_VALUE;
                            fc->flag_error_name = flag;
                            return false;
                        }
                        arg = flag_shift_args(&argc, &argv);
                    } else {
                        arg = equals;
                    }

                    static_assert(sizeof(unsigned long long int) == sizeof(uint64_t), "The original author designed this for x86_64 machine with the compiler that expects unsigned long long int and uint64_t to be the same thing, so they could use strtoull() function to parse it. Please adjust this code for your case and maybe even send the patch to upstream to make it work on a wider range of environments.");
                    char *endptr;
                    // TODO: replace strtoull with a custom solution
                    // That way we can get rid of the dependency on errno and static_assert
                    unsigned long long int result = strtoull(arg, &endptr, 10);

                    if (*endptr != '\0') {
                        fc->flag_error = FLAG_ERROR_INVALID_NUMBER;
                        fc->flag_error_name = flag;
                        return false;
                    }

                    if (result == ULLONG_MAX && errno == ERANGE) {
                        fc->flag_error = FLAG_ERROR_INTEGER_OVERFLOW;
                        fc->flag_error_name = flag;
                        return false;
                    }

                    fc->flags[i].val.as_uint64 = result;
                }
                break;

                case FLAG_SIZE: {
                    char *arg;
                    if (equals == NULL) {
                        if (argc == 0) {
                            fc->flag_error = FLAG_ERROR_NO_VALUE;
                            fc->flag_error_name = flag;
                            return false;
                        }
                        arg = flag_shift_args(&argc, &argv);
                    } else {
                        arg = equals;
                    }

                    static_assert(sizeof(unsigned long long int) == sizeof(size_t), "The original author designed this for x86_64 machine with the compiler that expects unsigned long long int and size_t to be the same thing, so they could use strtoull() function to parse it. Please adjust this code for your case and maybe even send the patch to upstream to make it work on a wider range of environments.");
                    char *endptr;
                    // TODO: replace strtoull with a custom solution
                    // That way we can get rid of the dependency on errno and static_assert
                    unsigned long long int result = strtoull(arg, &endptr, 10);

                    // TODO: handle more multiplicative suffixes like in dd(1). From the dd(1) man page:
                    // > N and BYTES may be followed by the following
                    // > multiplicative suffixes: c =1, w =2, b =512, kB =1000, K
                    // > =1024, MB =1000*1000, M =1024*1024, xM =M, GB
                    // > =1000*1000*1000, G =1024*1024*1024, and so on for T, P,
                    // > E, Z, Y.
                    if (strcmp(endptr, "K") == 0) {
                        result *= 1024;
                    } else if (strcmp(endptr, "M") == 0) {
                        result *= 1024*1024;
                    } else if (strcmp(endptr, "G") == 0) {
                        result *= 1024*1024*1024;
                    } else if (strcmp(endptr, "") != 0) {
                        fc->flag_error = FLAG_ERROR_INVALID_SIZE_SUFFIX;
                        fc->flag_error_name = flag;
                        // TODO: capability to report what exactly is the wrong suffix
                        return false;
                    }

                    if (result == ULLONG_MAX && errno == ERANGE) {
                        fc->flag_error = FLAG_ERROR_INTEGER_OVERFLOW;
                        fc->flag_error_name = flag;
                        return false;
                    }

                    fc->flags[i].val.as_size = result;
                }
                break;

                case COUNT_FLAG_TYPES:
                default: {
                    assert(0 && "unreachable");
                    exit(69);
                }
                }

                found = true;
            }
        }

        if (!found) {
            fc->flag_error = FLAG_ERROR_UNKNOWN;
            fc->flag_error_name = flag;
            return false;
        }
    }

    fc->rest_argc = argc;
    fc->rest_argv = argv;
    return true;
}

bool flag_parse(int argc, char **argv)
{
    return flag_c_parse(&flag_global_context, argc, argv);
}

void flag_c_print_options(void *c, FILE *stream)
{
    Flag_Context *fc = (Flag_Context *)c;
    for (size_t i = 0; i < fc->flags_count; ++i) {
        Flag *flag = &fc->flags[i];

        static_assert(COUNT_FLAG_TYPES == 6, "Exhaustive flag type defaults printing");
        switch (fc->flags[i].type) {
        case FLAG_LIST_MUT:
        case FLAG_LIST:
            fprintf(stream, "    -%s <str> ... -%s <str> ...\n", flag->name, flag->name);
            fprintf(stream, "        %s\n", flag->desc);
            break;
        case FLAG_BOOL:
            fprintf(stream, "    -%s\n", flag->name);
            fprintf(stream, "        %s\n", flag->desc);
            if (flag->def.as_bool) {
                fprintf(stream, "        Default: %s\n", flag->def.as_bool ? "true" : "false");
            }
            break;
        case FLAG_UINT64:
            fprintf(stream, "    -%s <int>\n", flag->name);
            fprintf(stream, "        %s\n", flag->desc);
            fprintf(stream, "        Default: %" PRIu64 "\n", flag->def.as_uint64);
            break;
        case FLAG_SIZE:
            fprintf(stream, "    -%s <int>\n", flag->name);
            fprintf(stream, "        %s\n", flag->desc);
            fprintf(stream, "        Default: %zu\n", flag->def.as_size);
            break;
        case FLAG_STR:
            fprintf(stream, "    -%s <str>\n", flag->name);
            fprintf(stream, "        %s\n", flag->desc);
            if (flag->def.as_str) {
                fprintf(stream, "        Default: %s\n", flag->def.as_str);
            }
            break;
        case COUNT_FLAG_TYPES:
        default:
            assert(0 && "unreachable");
            exit(69);
        }
    }
}

void flag_print_options(FILE *stream)
{
    flag_c_print_options(&flag_global_context, stream);
}

void flag_c_print_error(void *c, FILE *stream)
{
    Flag_Context *fc = (Flag_Context *)c;
    static_assert(COUNT_FLAG_ERRORS == 6, "Exhaustive flag error printing");
    switch (fc->flag_error) {
    case FLAG_NO_ERROR:
        // NOTE: don't call flag_print_error() if flag_parse() didn't return false, okay? ._.
        fprintf(stream, "Operation Failed Successfully! Please tell the developer of this software that they don't know what they are doing! :)");
        break;
    case FLAG_ERROR_UNKNOWN:
        fprintf(stream, "ERROR: -%s: unknown flag\n", fc->flag_error_name);
        break;
    case FLAG_ERROR_NO_VALUE:
        fprintf(stream, "ERROR: -%s: no value provided\n", fc->flag_error_name);
        break;
    case FLAG_ERROR_INVALID_NUMBER:
        fprintf(stream, "ERROR: -%s: invalid number\n", fc->flag_error_name);
        break;
    case FLAG_ERROR_INTEGER_OVERFLOW:
        fprintf(stream, "ERROR: -%s: integer overflow\n", fc->flag_error_name);
        break;
    case FLAG_ERROR_INVALID_SIZE_SUFFIX:
        fprintf(stream, "ERROR: -%s: invalid size suffix\n", fc->flag_error_name);
        break;
    case COUNT_FLAG_ERRORS:
    default:
        assert(0 && "unreachable");
        exit(69);
    }
}

void flag_print_error(FILE *stream)
{
    flag_c_print_error(&flag_global_context, stream);
}

#endif // FLAG_IMPLEMENTATION

/*
   Revision history:

     1.4.1 (2025-09-05) Fix -Wswitch-enum warning for GCC/Clang
     1.4.0 (2025-07-23) Add support for explicit flag contexts
                        Add Flag_List_Mut
                        Implement `-key=value` syntax for flag lists
     1.3.0 (2025-07-21) Add support for `-key=value` syntax (by @ej-shafran)
     1.2.1 (2025-07-04) flag_print_options: denote expected argument types
                        flag_print_options: indicate flag list usage more clearly
     1.2.0 (2025-05-31) Introduce FLAG_PUSH_DASH_DASH_BACK (by @nullnominal)
     1.1.0 (2025-05-09) Introduce flag list
     1.0.0 (2025-03-03) Initial release
                        Save program_name in the context

*/

// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
