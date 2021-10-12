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
// TODO: *_var function variants
// void flag_bool_var(bool *var, const char *name, bool def, const char *desc);
// void flag_bool_uint64(uint64_t *var, const char *name, bool def, const char *desc);
// etc.
// WARNING! *_var functions may break the flag_name() functionalitys

bool *flag_bool (const char *name, const char *desc);
uint8_t *flag_uint8 (const char *name, const char *desc);
int8_t *flag_int8 (const char *name, const char *desc);
uint16_t *flag_uint16 (const char *name, const char *desc);
int16_t *flag_int16 (const char *name, const char *desc);
uint32_t *flag_uint32 (const char *name, const char *desc);
int32_t *flag_int32 (const char *name, const char *desc);
uint64_t *flag_uint64 (const char *name, const char *desc);
int64_t *flag_int64 (const char *name, const char *desc);
size_t *flag_size (const char *name, const char *desc);
float *flag_float (const char *name, const char *desc);
double *flag_double (const char *name, const char *desc);
char **flag_str (const char *name, const char *desc);

#if __STDC_VERSION__ >= 201112L
#define flag_default(val, def) _Generic((val), \
        bool*: flag_bool_default, \
      int8_t*: flag_int8_default, \
     uint8_t*:  flag_uint8_default, \
     int16_t*: flag_int16_default, \
    uint16_t*: flag_uint16_default, \
     int32_t*: flag_int32_default, \
    uint32_t*: flag_uint32_default, \
     int64_t*: flag_int64_default, \
    uint64_t*: flag_uint64_default, \
       char**: flag_str_default, \
       float*: flag_float_default, \
      double*: flag_double_default, \
      default: ((void*)0) \
    )(val, def)
#elif __cplusplus
template <typename T> void flag_default(void *val, T def);
#endif

void flag_bool_default(bool *val, bool def);
void flag_uint8_default(uint8_t *val, uint8_t def);
void flag_int8_default(int8_t *val, int8_t def);
void flag_uint16_default(uint16_t *val, uint16_t def);
void flag_int16_default(int16_t *val, int16_t def);
void flag_uint32_default(uint32_t *val, uint32_t def);
void flag_int32_default(int32_t *val, int32_t def);
void flag_uint64_default(uint64_t *val, uint64_t def);
void flag_int64_default(int64_t *val, int64_t def);
void flag_size_default(size_t *val, size_t def);
void flag_float_default(float *val, float def);
void flag_double_default(double *val, double def);
void flag_str_default(char **val, char *def);

void flag_required(void *val, bool req);

char *flag_name(void *val);

int flag_rest_argc(void);
char **flag_rest_argv(void);

void flag_print_error(FILE *stream);
void flag_print_options(FILE *stream);


#endif // FLAG_H_

//////////////////////////////

#ifdef FLAG_IMPLEMENTATION

#define recast(var, type) (*((type*)&(var)))

typedef enum Flag_Type_Enum {
    FLAG_BOOL = 0,
    FLAG_UINT8,
    FLAG_INT8,
    FLAG_UINT16,
    FLAG_INT16,
    FLAG_UINT32,
    FLAG_INT32,
    FLAG_UINT64,
    FLAG_INT64,
    FLAG_SIZE,
    FLAG_FLOAT,
    FLAG_DOUBLE,
    FLAG_STR,
    COUNT_FLAG_TYPES,
} Flag_Type;

typedef enum Flag_Error_Enum {
    FLAG_NO_ERROR = 0,
    FLAG_ERROR_UNKNOWN,
    FLAG_ERROR_NO_VALUE,
    FLAG_ERROR_INVALID_NUMBER,
    FLAG_ERROR_INTEGER_OVERFLOW,
    FLAG_ERROR_INVALID_SIZE_SUFFIX,
    FLAG_ERROR_REQUIRED,
    COUNT_FLAG_ERRORS,
} Flag_Error;

typedef struct Flag_Struct {
    Flag_Type type;
    char *name;
    char *desc;
    bool req;
    uintptr_t val, def;
} Flag;

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif

typedef struct Flag_Context_Struct {
    Flag flags[FLAGS_CAP];
    size_t flags_count;

    Flag_Error flag_error;
    char *flag_error_name;

    int rest_argc;
    char **rest_argv;
} Flag_Context;

static Flag_Context flag_global_context;

Flag *flag_new(Flag_Type type, const char *name, const char *desc)
{
    Flag_Context *c = &flag_global_context;

    assert(c->flags_count < FLAGS_CAP);
    Flag *flag = &c->flags[c->flags_count++];
    memset(flag, 0, sizeof(*flag));
    flag->type = type;
    // NOTE: I won't touch them I promise Kappa
    flag->name = (char*) name;
    flag->desc = (char*) desc;
    return flag;
}


#define new_flag_impl(ftype, ctype, mname) \
ctype *flag_ ## mname (const char *name, const char *desc) \
{ \
    Flag *flag = flag_new(ftype, name, desc); \
    return &recast(flag->val, ctype); \
} \
\
void flag_ ## mname ## _default(ctype *val, ctype def) \
{ \
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val)); \
    recast(flag->val, ctype) = recast(flag->def, ctype) = def; \
}

#ifdef __cplusplus
template<typename T>
void flag_default(void *val, T def)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    flag->val = flag->def = recast(def, uintptr_t);
}
#endif

new_flag_impl(FLAG_BOOL, bool, bool)
new_flag_impl(FLAG_UINT8, uint8_t, uint8)
new_flag_impl(FLAG_INT8, int8_t, int8)
new_flag_impl(FLAG_UINT16, uint16_t, uint16)
new_flag_impl(FLAG_INT16, int16_t, int16)
new_flag_impl(FLAG_UINT32, uint32_t, uint32)
new_flag_impl(FLAG_INT32, int32_t, int32)
new_flag_impl(FLAG_UINT64, uint64_t, uint64)
new_flag_impl(FLAG_INT64, int64_t, int64)
new_flag_impl(FLAG_SIZE, size_t, size)
new_flag_impl(FLAG_FLOAT, float, float)
new_flag_impl(FLAG_DOUBLE, double, double)
new_flag_impl(FLAG_STR, char*, str)

void flag_required(void *val, bool req)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    flag->req = req;
}

char *flag_name(void *val)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    return flag->name;
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

#define flag_test_has_value() \
do { if (argc == 0) { \
    c->flag_error = FLAG_ERROR_NO_VALUE; \
    c->flag_error_name = flag; \
    return false; \
} } while (0)

#define flag_test_invalid_number() \
do { if (*endptr != '\0') { \
    c->flag_error = FLAG_ERROR_INVALID_NUMBER; \
    c->flag_error_name = flag; \
    return false; \
} } while (0)

#define flag_test_signed_overflow(MIN, MAX) \
do { if ((result <= MIN || result >= MAX) && errno == ERANGE) { \
    c->flag_error = FLAG_ERROR_INTEGER_OVERFLOW; \
    c->flag_error_name = flag; \
    return false; \
} } while (0)

#define flag_test_unsigned_overflow(MAX) \
do { if (result >= MAX && errno == ERANGE) { \
    c->flag_error = FLAG_ERROR_INTEGER_OVERFLOW; \
    c->flag_error_name = flag; \
    return false; \
} } while (0)

bool flag_parse(int argc, char **argv)
{
    Flag_Context *c = &flag_global_context;

    flag_shift_args(&argc, &argv);

    while (argc > 0) {
        char *flag = flag_shift_args(&argc, &argv);

        if (*flag != '-') {
            // NOTE: pushing flag back into args
            c->rest_argc = argc + 1;
            c->rest_argv = argv - 1;
            goto success;
        }

        if (strcmp(flag, "--") == 0) {
            // NOTE: but if it's the terminator we don't need to push it back
            c->rest_argc = argc;
            c->rest_argv = argv;
            goto success;
        }

        // NOTE: remove the dash
        flag += 1;

        bool found = false;
        for (size_t i = 0; i < c->flags_count; ++i) {
            if (strcmp(c->flags[i].name, flag) == 0) {
                static_assert(COUNT_FLAG_TYPES == 13, "Exhaustive flag type parsing");
                switch (c->flags[i].type) {
                case FLAG_BOOL: {
                    recast(c->flags[i].val, bool) = true;
                }
                break;

                // TODO: replace strtoumax, strtoimax in the following cases with a custom solution
                // That way we can get rid of the dependency on errno and static_assert

                case FLAG_INT8: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(char) == sizeof(int8_t), "Architecture mismatch");
                    char *endptr;
                    int8_t result = strtoimax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_signed_overflow(INT8_MIN, INT8_MAX);

                    recast(c->flags[i].val, int8_t) = result;
                }
                break;

                case FLAG_UINT8: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned char) == sizeof(uint8_t), "Architecture mismatch");
                    char *endptr;
                    uint8_t result = strtoumax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_unsigned_overflow(UINT8_MAX);

                    recast(c->flags[i].val, uint8_t) = result;
                }
                break;

                case FLAG_INT16: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(short) == sizeof(int16_t), "Architecture mismatch");
                    char *endptr;
                    int16_t result = strtoimax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_signed_overflow(INT16_MIN, INT16_MAX);

                    recast(c->flags[i].val, int16_t) = result;
                }
                break;

                case FLAG_UINT16: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned short) == sizeof(uint16_t), "Architecture mismatch");
                    char *endptr;
                    uint16_t result = strtoumax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_unsigned_overflow(UINT16_MAX);

                    recast(c->flags[i].val, uint16_t) = result;
                }
                break;

                case FLAG_INT32: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(int) == sizeof(int32_t), "Architecture mismatch");
                    char *endptr;
                    int32_t result = strtoimax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_signed_overflow(INT32_MIN, INT32_MAX);

                    recast(c->flags[i].val, int32_t) = result;
                }
                break;

                case FLAG_UINT32: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned int) == sizeof(uint32_t), "Architecture mismatch");
                    char *endptr;
                    uint32_t result = strtoumax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_unsigned_overflow(UINT32_MAX);

                    recast(c->flags[i].val, uint32_t) = result;
                }
                break;

                case FLAG_INT64: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(long long) == sizeof(int64_t), "Architecture mismatch");
                    char *endptr;
                    int64_t result = strtoimax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_signed_overflow(INT64_MIN, INT64_MAX);

                    recast(c->flags[i].val, int64_t) = result;
                }
                break;

                case FLAG_UINT64: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "Architecture mismatch");
                    char *endptr;
                    uint64_t result = strtoumax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_unsigned_overflow(UINT64_MAX);

                    recast(c->flags[i].val, uint64_t) = result;
                }
                break;

                case FLAG_SIZE: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    static_assert(sizeof(unsigned long long) == sizeof(size_t), "Architecture mismatch");
                    char *endptr;
                    size_t result = strtoumax(arg, &endptr, 10);

                    flag_test_invalid_number();
                    flag_test_unsigned_overflow(SIZE_MAX);

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
                        return false;
                    } 

                    flag_test_unsigned_overflow(SIZE_MAX);

                    recast(c->flags[i].val, size_t) = result;
                }
                break;

                case FLAG_FLOAT: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    char *endptr;
                    float result = strtof(arg, &endptr);
                    
                    // TODO: floating point overflow/underflow test? currently sets values > FLT_MAX to inf (-inf for < -FLT_MAX)
                    // is that what we want?
                    flag_test_invalid_number();

                    recast(c->flags[i].val, float) = result;
                }
                break;

                case FLAG_DOUBLE: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);

                    char *endptr;
                    double result = strtod(arg, &endptr);

                    // TODO: floating point overflow test? currently sets values > DBL_MAX to infinity (-inf for < -DBL_MAX)
                    // is that what we want?
                    flag_test_invalid_number();

                    recast(c->flags[i].val, double) = result;
                }
                break;

                case FLAG_STR: {
                    flag_test_has_value();
                    char *arg = flag_shift_args(&argc, &argv);
                    recast(c->flags[i].val, char*) = arg;
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

success:

    for (size_t i = 0; i < c->flags_count; ++i)
    {
        Flag f = c->flags[i];
        if(f.req && f.val == f.def)
        {
            c->flag_error = FLAG_ERROR_REQUIRED;
            c->flag_error_name = f.name;
            return false;
        }
    }

    return true;
}

#define opt_fprintf_def(code, type, value) \
do { if((type)flag->def) { fprintf(stream, "\n        Default: %" code, (value)); } } while(0)

void flag_print_options(FILE *stream)
{
    Flag_Context *c = &flag_global_context;
    for (size_t i = 0; i < c->flags_count; ++i) {
        Flag *flag = &c->flags[i];

        fprintf(stream, "    -%s%s\n        %s", flag->name, flag->req ? " (Required)" : "", flag->desc);
        static_assert(COUNT_FLAG_TYPES == 13, "Exhaustive flag type defaults printing");
        switch (c->flags[i].type) {
        case FLAG_BOOL:
            opt_fprintf_def("s", bool, (bool)flag->def ? "true" : "false"); break;
        case FLAG_INT8:
            opt_fprintf_def(PRIi8, int8_t, (int8_t)flag->def); break;
        case FLAG_UINT8:
            opt_fprintf_def(PRIu8, uint8_t, (uint8_t)flag->def); break;
        case FLAG_INT16:
            opt_fprintf_def(PRIi16, int16_t, (int16_t)flag->def); break;
        case FLAG_UINT16:
            opt_fprintf_def(PRIu16, uint16_t, (uint16_t)flag->def); break;
        case FLAG_INT32:
            opt_fprintf_def(PRIi32, int32_t, (int32_t)flag->def); break;
        case FLAG_UINT32:
            opt_fprintf_def(PRIu32, uint32_t, (uint32_t)flag->def); break;
        case FLAG_INT64:
            opt_fprintf_def(PRIi64, int64_t, (int64_t)flag->def); break;
        case FLAG_UINT64:
            opt_fprintf_def(PRIu64, uint64_t, (uint64_t)flag->def); break;
        case FLAG_SIZE:
            opt_fprintf_def("zu", size_t, (size_t)flag->def); break;
        case FLAG_STR:
            opt_fprintf_def("s", char*, (char*)flag->def); break;
        case FLAG_FLOAT:
            opt_fprintf_def("f", float, recast(flag->def, float)); break;
        case FLAG_DOUBLE:
            opt_fprintf_def("f", double, recast(flag->def, double)); break;
        default:
            assert(0 && "unreachable");
            exit(69);
        }
        fprintf(stream, "\n");
    }
}

void flag_print_error(FILE *stream)
{
    Flag_Context *c = &flag_global_context;
    static_assert(COUNT_FLAG_ERRORS == 7, "Exhaustive flag error printing");
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
    case FLAG_ERROR_REQUIRED:
        fprintf(stream, "ERROR: -%s: value is required\n", c->flag_error_name);
        break;
    case COUNT_FLAG_ERRORS:
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
