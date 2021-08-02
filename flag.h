// flag.h -- command-line flag parsing
//
//   Inspired by Go's flag module: https://pkg.go.dev/flag
//
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

// TODO: add support for -flag=x syntax
// TODO: stop parsing after the first non-flag argument or -- terminator
// Add ability to get the rest unparsed arguments
// TODO: *_var function variants
// void flag_bool_var(bool *var, const char *name, bool def, const char *desc);
// void flag_bool_uint64(uint64_t *var, const char *name, bool def, const char *desc);
// etc.
// WARNING! *_var functions may break the flag_name() functionality

char *flag_name(void *val);
bool *flag_bool(const char *name, bool def, const char *desc);
uint64_t *flag_uint64(const char *name, uint64_t def, const char *desc);
char **flag_str(const char *name, const char *def, const char *desc);
bool flag_parse(int argc, char **argv);
void flag_print_error(FILE *stream);
void flag_print_options(FILE *stream);

#endif // FLAG_H_

//////////////////////////////

#ifdef FLAG_IMPLEMENTATION

typedef enum {
    FLAG_BOOL,
    FLAG_UINT64,
    FLAG_STR,
} Flag_Type;

typedef union {
    char *as_str;
    uint64_t as_uint64;
    bool as_bool;
} Flag_Value;

typedef enum {
    FLAG_NO_ERROR = 0,
    FLAG_ERROR_UNKNOWN,
    FLAG_ERROR_NO_VALUE,
    FLAG_ERROR_INVALID_NUMBER,
    FLAG_ERROR_INTEGER_OVERFLOW,
} Flag_Error;

typedef struct {
    Flag_Type type;
    char *name;
    char *desc;
    Flag_Value val;
    Flag_Value def;
} Flag;

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif

static Flag flags[FLAGS_CAP];
static size_t flags_count = 0;

static Flag_Error flag_error = FLAG_NO_ERROR;
static char *flag_error_name = NULL;

Flag *flag_new(Flag_Type type, const char *name, const char *desc)
{
    assert(flags_count < FLAGS_CAP);
    Flag *flag = &flags[flags_count++];
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

bool *flag_bool(const char *name, bool def, const char *desc)
{
    Flag *flag = flag_new(FLAG_BOOL, name, desc);
    flag->def.as_bool = def;
    flag->val.as_bool = def;
    return &flag->val.as_bool;
}

uint64_t *flag_uint64(const char *name, uint64_t def, const char *desc)
{
    Flag *flag = flag_new(FLAG_UINT64, name, desc);
    flag->val.as_uint64 = def;
    flag->def.as_uint64 = def;
    return &flag->val.as_uint64;
}

char **flag_str(const char *name, const char *def, const char *desc)
{
    Flag *flag = flag_new(FLAG_STR, name, desc);
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

bool flag_parse(int argc, char **argv)
{
    flag_shift_args(&argc, &argv);

    while (argc > 0) {
        char *flag = flag_shift_args(&argc, &argv);

        if (*flag != '-') {
            flag_error = FLAG_ERROR_UNKNOWN;
            flag_error_name = flag;
            return false;
        }

        flag += 1;

        bool found = false;
        for (size_t i = 0; i < flags_count; ++i) {
            if (strcmp(flags[i].name, flag) == 0) {
                switch (flags[i].type) {
                case FLAG_BOOL: {
                    flags[i].val.as_bool = true;
                }
                break;

                case FLAG_STR: {
                    if (argc == 0) {
                        flag_error = FLAG_ERROR_NO_VALUE;
                        flag_error_name = flag;
                        return false;
                    }
                    char *arg = flag_shift_args(&argc, &argv);
                    flags[i].val.as_str = arg;
                }
                break;

                case FLAG_UINT64: {
                    if (argc == 0) {
                        flag_error = FLAG_ERROR_NO_VALUE;
                        flag_error_name = flag;
                        return false;
                    }
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned long long int) == sizeof(uint64_t), "The original author designed this for x86_64 machine with the compiler that expects unsigned long long int and uint64_t to be the same thing, so they could use strtoull() function to parse it. Please adjust this code for your case and maybe even send the patch to upstream to make it work on a wider range of environments.");
                    char *endptr;
                    // TODO: replace strtoull with a custom solution
                    // That way we can get rid of the dependency on errno and static_assert
                    unsigned long long int result = strtoull(arg, &endptr, 10);

                    if (arg == endptr || *endptr != '\0') {
                        flag_error = FLAG_ERROR_INVALID_NUMBER;
                        flag_error_name = flag;
                        return false;
                    }
                    if (result == ULLONG_MAX && errno == ERANGE) {
                        flag_error = FLAG_ERROR_INTEGER_OVERFLOW;
                        flag_error_name = flag;
                        return false;
                    }

                    flags[i].val.as_uint64 = result;
                }
                break;

                default: {
                    assert(0 && "unreachable");
                    exit(69);
                }
                }

                found = true;
            }
        }

        if (!found) {
            flag_error = FLAG_ERROR_UNKNOWN;
            flag_error_name = flag;
            return false;
        }
    }

    return true;
}

void flag_print_options(FILE *stream)
{
    for (size_t i = 0; i < flags_count; ++i) {
        fprintf(stream, "    -%s\n", flags[i].name);
        fprintf(stream, "        %s.\n", flags[i].desc);
        switch (flags[i].type) {
        case FLAG_BOOL:
            fprintf(stream, "        Default: %s\n", flags[i].val.as_bool ? "true" : "false");
            break;
        case FLAG_UINT64:
            fprintf(stream, "        Default: %" PRIu64 "\n", flags[i].val.as_uint64);
            break;
        case FLAG_STR:
            fprintf(stream, "        Default: %s\n", flags[i].val.as_str);
            break;
        default:
            assert(0 && "unreachable");
            exit(69);
        }
    }
}

void flag_print_error(FILE *stream)
{
    switch (flag_error) {
    case FLAG_NO_ERROR:
        // NOTE: don't call flag_print_error() if flag_parse() didn't return false, okay? ._.
        fprintf(stream, "Operation Failed Successfully! Please tell the developer of this software that they don't know what they are doing! :)");
        break;
    case FLAG_ERROR_UNKNOWN:
        fprintf(stream, "ERROR: -%s: unknown flag\n", flag_error_name);
        break;
    case FLAG_ERROR_NO_VALUE:
        fprintf(stream, "ERROR: -%s: no value provided\n", flag_error_name);
        break;
    case FLAG_ERROR_INVALID_NUMBER:
        fprintf(stream, "ERROR: -%s: invalid number\n", flag_error_name);
        break;
    case FLAG_ERROR_INTEGER_OVERFLOW:
        fprintf(stream, "ERROR: -%s: integer overflow\n", flag_error_name);
        break;
    default:
        assert(0 && "unreachable");
        exit(69);
    }
}

#endif
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
