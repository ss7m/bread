#ifndef BRD_VALUE_H
#define BRD_VALUE_H

enum brd_value_type {
        BRD_VAL_NUM,
        BRD_VAL_STRING, /* string constant */
        BRD_VAL_BOOL,
        BRD_VAL_UNIT,
        BRD_VAL_BUILTIN,
};

struct brd_value {
        enum brd_value_type vtype;
        union {
                long double num;
                char *string;
                int boolean;
                int builtin;
        } as;
};

#ifdef DEBUG
void brd_value_debug(struct brd_value *value);
#endif

void brd_value_coerce_num(struct brd_value *value);
int brd_value_coerce_string(struct brd_value *value);
int brd_value_truthify(struct brd_value *value);
int brd_value_compare(struct brd_value *a, struct brd_value *b);
void brd_value_call(struct brd_value *f, struct brd_value *args, size_t num_args, struct brd_value *out);

enum brd_builtin {
        BRD_BUILTIN_WRITE,
        BRD_BUILTIN_WRITELN,
        BRD_BUILTIN_READLN,
        BRD_BUILTIN_LENGTH,
        BRD_BUILTIN_TYPEOF,
        BRD_BUILTIN_SYSTEM,
        BRD_NUM_BUILTIN,
};

typedef void (*builtin_fn_dec)(
        struct brd_value *args,
        size_t num_args,
        struct brd_value *out
);

extern const builtin_fn_dec builtin_function[BRD_NUM_BUILTIN];

extern const char *builtin_name[BRD_NUM_BUILTIN];

enum brd_builtin brd_lookup_builtin(char *builtin);

#endif
