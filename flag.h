// flag.h -- v1.0.0 -- command-line flag parsing
//
//   Inspired by Go's flag module: https://pkg.go.dev/flag
//
#ifndef FLAG_H_
#define FLAG_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

// TODO: add support for -flag=x syntax
// TODO: *_var function variants
// void flag_bool_var(bool *var, const char *name, bool def, const char *desc);
// void flag_bool_uint64(uint64_t *var, const char *name, bool def, const char *desc);
// etc.
// WARNING! *_var functions may break the flag_name() functionality

char *flag_name(void *val);
void flag_add_alias(void *val, const char *alias);
bool *flag_bool_null(const char *name, bool def, const char *desc, ...);
#define flag_bool(name, def, desc) flag_bool_null(name, def, desc, NULL)
#define flag_bool_aliases(name, def, desc, ...)                                        \
  flag_bool_null(name, def, desc, __VA_ARGS__, NULL)
uint64_t *flag_uint64_null(const char *name, uint64_t def, const char *desc, ...);
#define flag_uint64(name, def, desc) flag_uint64_null(name, def, desc, NULL)
#define flag_uint64_aliases(name, def, desc, ...)                              \
  flag_uint64_null(name, def, desc, __VA_ARGS__, NULL)
size_t *flag_size_null(const char *name, uint64_t def, const char *desc, ...);
#define flag_size(name, def, desc) flag_size_null(name, def, desc, NULL)
#define flag_size_aliases(name, def, desc, ...) \
    flag_size_null(name, def, desc, __VA_ARGS__, NULL)
char **flag_str_null(const char *name, const char *def, const char *desc, ...);
#define flag_str(name, def, desc) flag_str_null(name, def, desc, NULL)
#define flag_str_aliases(name, def, desc, ...)                                         \
  flag_str_null(name, def, desc, __VA_ARGS__, NULL)
bool flag_parse(int argc, char **argv);
int flag_rest_argc(void);
char **flag_rest_argv(void);
void flag_print_error(FILE *stream);
void flag_print_options(FILE *stream);

#endif // FLAG_H_

//////////////////////////////

#ifdef FLAG_IMPLEMENTATION

typedef enum {
    FLAG_BOOL = 0,
    FLAG_UINT64,
    FLAG_SIZE,
    FLAG_STR,
    COUNT_FLAG_TYPES,
} Flag_Type;

static_assert(COUNT_FLAG_TYPES == 4, "Exhaustive Flag_Value definition");
typedef union {
    char *as_str;
    uint64_t as_uint64;
    bool as_bool;
    size_t as_size;
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

#ifndef ALIAS_CAP
#define ALIAS_CAP 12
#endif // ALIAS_CAP

typedef struct {
    Flag_Type type;
    char *name;
    const char *aliases[ALIAS_CAP];
    size_t alias_count;
    char *desc;
    Flag_Value val;
    Flag_Value def;
} Flag;

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif

typedef struct {
    Flag flags[FLAGS_CAP];
    size_t flags_count;

    Flag_Error flag_error;
    char *flag_error_name;

    const char *program_name;

    int rest_argc;
    char **rest_argv;
} Flag_Context;

static Flag_Context flag_global_context;

Flag *flag_new(Flag_Type type, const char *name, const char *desc, va_list aliases)
{
    Flag_Context *c = &flag_global_context;

    assert(c->flags_count < FLAGS_CAP);
    Flag *flag = &c->flags[c->flags_count++];
    memset(flag, 0, sizeof(*flag));
    flag->type = type;
    // NOTE: I won't touch them I promise Kappa
    flag->name = (char*) name;
    flag->desc = (char*) desc;
    const char *alias = va_arg(aliases, const char *);
    while (alias != NULL) {
        assert(flag->alias_count <= ALIAS_CAP);
        flag->aliases[flag->alias_count++] = alias;
        alias = va_arg(aliases, const char *);
    }
    return flag;
}

char *flag_name(void *val)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    return flag->name;
}

void flag_add_alias(void *val, const char *alias)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    assert(flag->alias_count + 1 <= ALIAS_CAP);
    flag->aliases[flag->alias_count++] = alias;
}

bool *flag_bool_null(const char *name, bool def, const char *desc, ...)
{
    va_list aliases;
    va_start(aliases, desc);
    Flag *flag = flag_new(FLAG_BOOL, name, desc, aliases);
    flag->def.as_bool = def;
    flag->val.as_bool = def;
    return &flag->val.as_bool;
}

uint64_t *flag_uint64_null(const char *name, uint64_t def, const char *desc, ...)
{
    va_list aliases;
    va_start(aliases, desc);
    Flag *flag = flag_new(FLAG_UINT64, name, desc, aliases);
    flag->val.as_uint64 = def;
    flag->def.as_uint64 = def;
    return &flag->val.as_uint64;
}

size_t *flag_size_null(const char *name, uint64_t def, const char *desc, ...)
{
    va_list aliases;
    va_start(aliases, desc);
    Flag *flag = flag_new(FLAG_SIZE, name, desc, aliases);
    flag->val.as_size = def;
    flag->def.as_size = def;
    return &flag->val.as_size;
}

char **flag_str_null(const char *name, const char *def, const char *desc, ...)
{
    va_list aliases;
    va_start(aliases, desc);
    Flag *flag = flag_new(FLAG_STR, name, desc, aliases);
    flag->val.as_str = (char*) def;
    flag->def.as_str = (char*) def;
    return &flag->val.as_str;
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

char **flag_rest_argv(void)
{
    return flag_global_context.rest_argv;
}

const char *flag_program_name(void)
{
    return flag_global_context.program_name;
}

bool flag_parse(int argc, char **argv)
{
    Flag_Context *c = &flag_global_context;

    if (c->program_name == NULL) {
        c->program_name = flag_shift_args(&argc, &argv);
    }

    while (argc > 0) {
        char *flag = flag_shift_args(&argc, &argv);

        if (*flag != '-') {
            // NOTE: pushing flag back into args
            c->rest_argc = argc + 1;
            c->rest_argv = argv - 1;
            return true;
        }

        if (strcmp(flag, "--") == 0) {
            // NOTE: but if it's the terminator we don't need to push it back
            c->rest_argc = argc;
            c->rest_argv = argv;
            return true;
        }

        // NOTE: remove the dash
        flag += 1;

        bool found = false;
        for (size_t i = 0; i < c->flags_count; ++i) {
            bool is_name = strcmp(c->flags[i].name, flag) == 0;
            bool is_alias = false;
            if (!is_name) {
                for (size_t j = 0; !is_alias && j < c->flags[i].alias_count; ++j) {
                    if (strcmp(c->flags[i].aliases[j], flag) == 0)
                        is_alias = true;
                }
            }
            if (is_name || is_alias) {
                static_assert(COUNT_FLAG_TYPES == 4, "Exhaustive flag type parsing");
                switch (c->flags[i].type) {
                case FLAG_BOOL: {
                    c->flags[i].val.as_bool = true;
                }
                break;

                case FLAG_STR: {
                    if (argc == 0) {
                        c->flag_error = FLAG_ERROR_NO_VALUE;
                        c->flag_error_name = flag;
                        return false;
                    }
                    char *arg = flag_shift_args(&argc, &argv);
                    c->flags[i].val.as_str = arg;
                }
                break;

                case FLAG_UINT64: {
                    if (argc == 0) {
                        c->flag_error = FLAG_ERROR_NO_VALUE;
                        c->flag_error_name = flag;
                        return false;
                    }
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned long long int) == sizeof(uint64_t), "The original author designed this for x86_64 machine with the compiler that expects unsigned long long int and uint64_t to be the same thing, so they could use strtoull() function to parse it. Please adjust this code for your case and maybe even send the patch to upstream to make it work on a wider range of environments.");
                    char *endptr;
                    // TODO: replace strtoull with a custom solution
                    // That way we can get rid of the dependency on errno and static_assert
                    unsigned long long int result = strtoull(arg, &endptr, 10);

                    if (*endptr != '\0') {
                        c->flag_error = FLAG_ERROR_INVALID_NUMBER;
                        c->flag_error_name = flag;
                        return false;
                    }

                    if (result == ULLONG_MAX && errno == ERANGE) {
                        c->flag_error = FLAG_ERROR_INTEGER_OVERFLOW;
                        c->flag_error_name = flag;
                        return false;
                    }

                    c->flags[i].val.as_uint64 = result;
                }
                break;

                case FLAG_SIZE: {
                    if (argc == 0) {
                        c->flag_error = FLAG_ERROR_NO_VALUE;
                        c->flag_error_name = flag;
                        return false;
                    }
                    char *arg = flag_shift_args(&argc, &argv);

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
                        c->flag_error = FLAG_ERROR_INVALID_SIZE_SUFFIX;
                        c->flag_error_name = flag;
                        // TODO: capability to report what exactly is the wrong suffix
                        return false;
                    }

                    if (result == ULLONG_MAX && errno == ERANGE) {
                        c->flag_error = FLAG_ERROR_INTEGER_OVERFLOW;
                        c->flag_error_name = flag;
                        return false;
                    }

                    c->flags[i].val.as_size = result;
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
            c->flag_error = FLAG_ERROR_UNKNOWN;
            c->flag_error_name = flag;
            return false;
        }
    }

    c->rest_argc = argc;
    c->rest_argv = argv;
    return true;
}

void flag_print_options(FILE *stream)
{
    Flag_Context *c = &flag_global_context;
    for (size_t i = 0; i < c->flags_count; ++i) {
        Flag *flag = &c->flags[i];

        fprintf(stream, "    -%s", flag->name);
        for (size_t j = 0; j < flag->alias_count; ++j)
            fprintf(stream, ", -%s", flag->aliases[j]);
        fprintf(stream, "\n");
        fprintf(stream, "        %s\n", flag->desc);
        static_assert(COUNT_FLAG_TYPES == 4, "Exhaustive flag type defaults printing");
        switch (c->flags[i].type) {
        case FLAG_BOOL:
            if (flag->def.as_bool) {
                fprintf(stream, "        Default: %s\n", flag->def.as_bool ? "true" : "false");
            }
            break;
        case FLAG_UINT64:
            fprintf(stream, "        Default: %" PRIu64 "\n", flag->def.as_uint64);
            break;
        case FLAG_SIZE:
            fprintf(stream, "        Default: %zu\n", flag->def.as_size);
            break;
        case FLAG_STR:
            if (flag->def.as_str) {
                fprintf(stream, "        Default: %s\n", flag->def.as_str);
            }
            break;
        default:
            assert(0 && "unreachable");
            exit(69);
        }
    }
}

void flag_print_error(FILE *stream)
{
    Flag_Context *c = &flag_global_context;
    static_assert(COUNT_FLAG_ERRORS == 6, "Exhaustive flag error printing");
    switch (c->flag_error) {
    case FLAG_NO_ERROR:
        // NOTE: don't call flag_print_error() if flag_parse() didn't return false, okay? ._.
        fprintf(stream, "Operation Failed Successfully! Please tell the developer of this software that they don't know what they are doing! :)");
        break;
    case FLAG_ERROR_UNKNOWN:
        fprintf(stream, "ERROR: -%s: unknown flag\n", c->flag_error_name);
        break;
    case FLAG_ERROR_NO_VALUE:
        fprintf(stream, "ERROR: -%s: no value provided\n", c->flag_error_name);
        break;
    case FLAG_ERROR_INVALID_NUMBER:
        fprintf(stream, "ERROR: -%s: invalid number\n", c->flag_error_name);
        break;
    case FLAG_ERROR_INTEGER_OVERFLOW:
        fprintf(stream, "ERROR: -%s: integer overflow\n", c->flag_error_name);
        break;
    case FLAG_ERROR_INVALID_SIZE_SUFFIX:
        fprintf(stream, "ERROR: -%s: invalid size suffix\n", c->flag_error_name);
        break;
    case COUNT_FLAG_ERRORS:
    default:
        assert(0 && "unreachable");
        exit(69);
    }
}

#endif // FLAG_IMPLEMENTATION

/*
   Revision history:

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
