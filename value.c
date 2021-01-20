#include "common.h"
#include "value.h"

#include <math.h>

#ifdef DEBUG
void
brd_value_debug(struct brd_value *value)
{
        printf(" ======== ");
        switch (value->vtype) {
        case BRD_VAL_NUM:
                printf("%Lf\n", value->as.num);
                break;
        case BRD_VAL_STRING:
                printf("%s\n", value->as.string);
                break;
        case BRD_VAL_BOOL:
                printf("%s\n", value->as.boolean ? "true" : "false");
                break;
        case BRD_VAL_UNIT:
                printf("unit\n");
                break;
        case BRD_VAL_BUILTIN:
                printf("<< builtin: %s >>", builtin_name[value->as.builtin]);
                break;
        }
}
#endif

void
brd_value_coerce_num(struct brd_value *value)
{
        value->vtype = BRD_VAL_NUM;
        switch (value->vtype) {
        case BRD_VAL_NUM:
                break;
        case BRD_VAL_STRING:
                value->as.num = strtold(value->as.string, NULL);
                break;
        case BRD_VAL_BOOL:
                value->as.num = value->as.boolean ? 1 : 0;
                break;
        case BRD_VAL_UNIT:
                value->as.num = 0;
                break;
        case BRD_VAL_BUILTIN:
                BARF("builtin functions can't be coerced to a number");
                break;
        }
}

int
brd_value_coerce_string(struct brd_value *value)
{
        /* return true if string is malloced */
        int malloced = false;
        char *string = "";
        switch (value->vtype) {
        case BRD_VAL_NUM:
                string = malloc(sizeof(char) * 75); /* that's enough, right? */
                sprintf(string, "%Lg", value->as.num);
                malloced = true;
                break;
        case BRD_VAL_STRING:
                string = value->as.string;
                break;
        case BRD_VAL_BOOL:
                string = value->as.boolean ? "true" : "false";
                break;
        case BRD_VAL_UNIT:
                string = "unit";
                break;
        case BRD_VAL_BUILTIN:
                string = builtin_name[value->as.builtin];
                break;
        }
        value->vtype = BRD_VAL_STRING;
        value->as.string = string;
        return malloced;
}

int
brd_value_truthify(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                return value->as.num != 0;
        case BRD_VAL_STRING:
                return value->as.string[0] != '\0';
        case BRD_VAL_BOOL:
                return value->as.boolean;
        case BRD_VAL_UNIT:
                return false;
        case BRD_VAL_BUILTIN:
                return true;
        }
        BARF("what?");
        return false;
}

int
brd_value_compare(struct brd_value *a, struct brd_value *b)
{
        switch (a->vtype) {
        case BRD_VAL_NUM:
                brd_value_coerce_num(b);
                return (int) (a->as.num - b->as.num);
        case BRD_VAL_STRING:
                switch (b->vtype) {
                case BRD_VAL_NUM:
                        brd_value_coerce_num(a);
                        return (int) (a->as.num - b->as.num);
                case BRD_VAL_STRING:
                        return strcmp(a->as.string, b->as.string);
                case BRD_VAL_BOOL:
                        /* This is obviously a very good idea */
                        return strcmp(a->as.string, b->as.boolean ? "true" : "false");
                case BRD_VAL_UNIT:
                        return a->as.string[0] != '\0';
                case BRD_VAL_BUILTIN:
                        BARF("can't compare functions");
                        break;
                }
                break;
        case BRD_VAL_BOOL:
                switch (b->vtype) {
                case BRD_VAL_NUM:
                        brd_value_coerce_num(a);
                        return (int) (a->as.num - b->as.num);
                case BRD_VAL_STRING:
                        /* Again, this is the obvious thing to do there */
                        return strcmp(a->as.boolean ? "true" : "false", b->as.string);
                case BRD_VAL_BOOL:
                        return a->as.boolean - b->as.boolean;
                case BRD_VAL_UNIT:
                        return a->as.boolean;
                case BRD_VAL_BUILTIN:
                        BARF("can't compare functions");
                        break;
                }
                break;
        case BRD_VAL_UNIT:
                switch (b->vtype) {
                case BRD_VAL_NUM:
                        return -fabsl(b->as.num);
                case BRD_VAL_STRING:
                        return -(b->as.string[0] != '\0');
                case BRD_VAL_BOOL:
                        return -b->as.boolean;
                case BRD_VAL_UNIT:
                        return 0;
                case BRD_VAL_BUILTIN:
                        BARF("can't compare functions");
                        break;
                }
                break;
        case BRD_VAL_BUILTIN:
                BARF("can't compare functions");
                break;
        }
        BARF("what?");
        return -1;
}

void
brd_value_call(struct brd_value *f, struct brd_value *args, size_t num_args, struct brd_value *out)
{
        printf("Placeholder Function Call");
        out->vtype = BRD_VAL_UNIT;
        (void)f;(void)args;(void)num_args;(void)out;
}

static void
_builtin_write(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        out->vtype = BRD_VAL_UNIT;
        BARF("finish this");
        (void)args;(void)num_args;(void)out;
}

static void
_builtin_writeln(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        _builtin_write(args, num_args, out);
        printf("\n");
}

static void
_builtin_readln(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        BARF("finish this");
        (void)args;(void)num_args;(void)out;
}

static void
_builtin_length(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        BARF("finish this");
        (void)args;(void)num_args;(void)out;
}

static void
_builtin_typeof(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        BARF("finish this");
        (void)args;(void)num_args;(void)out;
}

static void
_builtin_system(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        BARF("finish this");
        (void)args;(void)num_args;(void)out;
}

enum brd_builtin brd_lookup_builtin(char *builtin)
{
        for (int i = 0; i < BRD_NUM_BUILTIN; i++) {
                if (strcmp(builtin_name[i], builtin) == 0) {
                        return i;
                }
        }

        BARF("Unknown builtin: %s", builtin);
}

const builtin_fn_dec builtin_function[BRD_NUM_BUILTIN] = {
        [BRD_BUILTIN_WRITE] = _builtin_write,
        [BRD_BUILTIN_WRITELN] = _builtin_writeln,
        [BRD_BUILTIN_READLN] = _builtin_readln,
        [BRD_BUILTIN_LENGTH] = _builtin_length,
        [BRD_BUILTIN_TYPEOF] = _builtin_typeof,
        [BRD_BUILTIN_SYSTEM] = _builtin_system,
};

const char *builtin_name[BRD_NUM_BUILTIN] = {
        [BRD_BUILTIN_WRITE] = "write",
        [BRD_BUILTIN_WRITELN] = "writeln",
        [BRD_BUILTIN_READLN] = "readln",
        [BRD_BUILTIN_LENGTH] = "length",
        [BRD_BUILTIN_TYPEOF] = "typeof",
        [BRD_BUILTIN_SYSTEM] = "system",
};

