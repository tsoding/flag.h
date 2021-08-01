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

typedef enum {
    FLAG_BOOL,
    FLAG_UINT64,
    FLAG_STR,
} Flag_Type;

// TODO: it does not really make sense for a flag to have any other defaults by false
// And since false is the default by default, maybe there is no need to print it in help
bool *flag_bool(const char *name, bool def, const char *desc);
uint64_t *flag_uint64(const char *name, uint64_t def, const char *desc);
void flag_uint64_range(uint64_t *flag, uint64_t min, uint64_t max);
char **flag_str(const char *name, char *def, const char *desc);
// TODO: flag_parse failures should be recoverable
void flag_parse(int argc, char **argv);
void flag_print_help(FILE *stream);

#endif // FLAG_H_

//////////////////////////////

#ifdef FLAG_IMPLEMENTATION

typedef enum {
    DATA_VAL = 0,
    DATA_DEF = 1,
    DATA_MIN = 2,
    DATA_MAX = 3,
} Flag_Data;

// TODO: figure out how to introduce mandatory flags (if its possible)
// If the failures of flag_parse() are recoverable it will be possible,
// but it may overcomplicated the interface
typedef struct {
    Flag_Type type;
    const char *name;
    const char *desc;
    bool provided;
    uintptr_t data[4];
} Flag;

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif

#ifndef FLAGS_TMP_STR_CAP
#define FLAGS_TMP_STR_CAP 1024
#endif

static Flag flags[FLAGS_CAP];
static size_t flags_count = 0;

// TODO: get rid of flags_tmp_str
static char flags_tmp_str[FLAGS_TMP_STR_CAP];
static size_t flags_tmp_str_size = 0;

Flag *flag_new(Flag_Type type, const char *name, const char *desc)
{
    assert(flags_count < FLAGS_CAP);
    Flag *flag = &flags[flags_count++];
    memset(flag, 0, sizeof(*flag));
    flag->type = type;
    flag->name = name;
    flag->desc = desc;
    return flag;
}

bool *flag_bool(const char *name, bool def, const char *desc)
{
    Flag *flag = flag_new(FLAG_BOOL, name, desc);
    *((bool*) &flag->data[DATA_DEF]) = def;
    *((bool*) &flag->data[DATA_VAL]) = def;
    return (bool*) &flag->data[DATA_VAL];
}

uint64_t *flag_uint64(const char *name, uint64_t def, const char *desc)
{
    Flag *flag = flag_new(FLAG_UINT64, name, desc);
    *((uint64_t*) &flag->data[DATA_DEF]) = def;
    *((uint64_t*) &flag->data[DATA_VAL]) = def;
    *((uint64_t*) &flag->data[DATA_MIN]) = 0;
    *((uint64_t*) &flag->data[DATA_MAX]) = UINT64_MAX;
    return (uint64_t*) &flag->data[DATA_VAL];
}

void flag_uint64_range(uint64_t *flag, uint64_t min, uint64_t max)
{
    assert(min <= max);
    static_assert(sizeof(uint64_t) == sizeof(uintptr_t), "This hack will only work if the size of uint64_t and uintptr_t is the same");
    flag[DATA_MIN] = min;
    flag[DATA_MAX] = max;
}

char **flag_str(const char *name, char *def, const char *desc)
{
    Flag *flag = flag_new(FLAG_STR, name, desc);
    *((char **)&flag->data[DATA_DEF]) = def;
    *((char **)&flag->data[DATA_VAL]) = def;
    return (char **)&flag->data[DATA_VAL];
}

static char *flag_shift_args(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

void flag_parse(int argc, char **argv)
{
    flag_shift_args(&argc, &argv);

    while (argc > 0) {
        char *flag = flag_shift_args(&argc, &argv);

        if (*flag != '-') {
            fprintf(stderr, "ERROR: -%s: unknown flag\n", flag);
            exit(1);
        }

        flag += 1;

        for (size_t i = 0; i < flags_count; ++i) {
            if (strcmp(flags[i].name, flag) == 0) {
                if (flags[i].provided) {
                    // TODO: should we introduce some sort of an option that allows to provide repeat the same flag?
                    // How do we handle it? Override the original flag? Collect into a list?
                    fprintf(stderr, "ERROR: -%s: provided twice\n", flag);
                    exit(1);
                }
                
                switch (flags[i].type) {
                case FLAG_BOOL: {
                    *(bool*)&flags[i].data = true;
                }
                break;

                case FLAG_STR: {
                    if (argc == 0) {
                        fprintf(stderr, "ERROR: -%s: no argument provided\n", flag);
                        exit(1);
                    }
                    char *arg = flag_shift_args(&argc, &argv);
                    *(char**)&flags[i].data = arg;
                }
                break;

                case FLAG_UINT64: {
                    if (argc == 0) {
                        fprintf(stderr, "ERROR: -%s: no argument provided\n", flag);
                        exit(1);
                    }
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned long long int) == sizeof(uint64_t), "The original author designed this for x86_64 machine with the compiler that expects unsigned long long int and uint64_t to be the same thing, so they could use strtoull() function to parse it. Please adjust this code for your case and maybe even send the patch to upstream to make it work on a wider range of environments.");
                    char *endptr;
                    unsigned long long int result = strtoull(arg, &endptr, 10);
                    if (arg == endptr || *endptr != '\0') {
                        fprintf(stderr, "ERROR: -%s: not a valid number\n", flag);
                        exit(1);
                    }
                    if (result == ULLONG_MAX && errno == ERANGE) {
                        fprintf(stderr, "ERROR: -%s: 64 bit unsigned integer overflow\n", flag);
                        exit(1);
                    }

                    uint64_t val = result;
                    uint64_t min = *(uint64_t*)&flags[i].data[DATA_MIN];
                    uint64_t max = *(uint64_t*)&flags[i].data[DATA_MAX];

                    if (!(min <= val && val <= max)) {
                        fprintf(stderr, "ERROR: -%s: outside of the [%"PRIu64"..%"PRIu64"] range\n", flag, min, max);
                        exit(1);
                    }

                    *(uint64_t*)&flags[i].data[DATA_VAL] = val;
                }
                break;

                default: {
                    assert(0 && "unreachable");
                    exit(69);
                }
                }

                flags[i].provided = true;
            }
        }
    }
}

static char *flag_show_data(Flag_Type type, uintptr_t data)
{
    switch (type) {
    case FLAG_BOOL:
        return (*(bool*) &data) ? "true" : "false";

    case FLAG_UINT64: {
        int n = snprintf(NULL, 0, "%"PRIu64, *(uint64_t*) &data);
        assert(n >= 0);
        assert(flags_tmp_str_size + n + 1 <= FLAGS_TMP_STR_CAP);
        int m = snprintf(flags_tmp_str + flags_tmp_str_size,
                         FLAGS_TMP_STR_CAP - flags_tmp_str_size,
                         "%"PRIu64,
                         *(uint64_t*) &data);
        assert(n == m);
        char *result = flags_tmp_str + flags_tmp_str_size;
        flags_tmp_str_size += n + 1;
        return result;
    }

    case FLAG_STR:
        return *(char**) &data;
    }

    assert(0 && "unreachable");
    exit(69);
}

void flag_print_help(FILE *stream)
{
    for (size_t i = 0; i < flags_count; ++i) {
        fprintf(stream, "    -%s\n", flags[i].name);
        fprintf(stream, "        %s.\n", flags[i].desc);
        fprintf(stream, "        Default: %s\n", flag_show_data(flags[i].type, flags[i].data[DATA_DEF]));
        if (flags[i].type == FLAG_UINT64) {
            fprintf(stream, "        Range: [%s..%s]\n",
                    flag_show_data(flags[i].type, flags[i].data[DATA_MIN]),
                    flag_show_data(flags[i].type, flags[i].data[DATA_MAX]));
        }

        flags_tmp_str_size = 0;
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
