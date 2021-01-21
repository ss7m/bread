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
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        printf("%s\n", value->as.heap->as.string);
                }
        }
}
#endif

int brd_value_is_string(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_STRING:
                return true;
        case BRD_VAL_HEAP:
                return value->as.heap->htype == BRD_HEAP_STRING;
        default:
                return false;
        }
}

void
brd_value_coerce_num(struct brd_value *value)
{
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
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        value->as.num = strtold(value->as.heap->as.string, NULL);
                }
        }
        value->vtype = BRD_VAL_NUM;
}

int
brd_value_coerce_string(struct brd_value *value)
{
        /* return true if new allocation was made */
        char *string = "";
        switch (value->vtype) {
        case BRD_VAL_NUM:
                value->vtype = BRD_VAL_HEAP;
                string = malloc(sizeof(char) * 50); /* that's enough, right? */
                sprintf(string, "%Lg", value->as.num);
                value->as.heap = malloc(sizeof(struct brd_heap_entry));
                value->as.heap->htype = BRD_HEAP_STRING;
                value->as.heap->as.string = string;
                return true;
        case BRD_VAL_STRING:
                return false;
        case BRD_VAL_BOOL:
                value->vtype = BRD_VAL_STRING;
                value->as.string = value->as.boolean ? "true" : "false";
                return false;
        case BRD_VAL_UNIT:
                value->vtype = BRD_VAL_STRING;
                value->as.string = "unit";
                return false;
        case BRD_VAL_BUILTIN:
                value->vtype = BRD_VAL_STRING;
                value->as.string = builtin_name[value->as.builtin];
                return false;
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                        case BRD_HEAP_STRING:
                                return false;
                }
        }
        BARF("what?");
        return -1;
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
        case BRD_VAL_HEAP:
                switch(value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        return value->as.heap->as.string[0] != '\0';
                }
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
                case BRD_VAL_HEAP:
                        switch (b->as.heap->htype) {
                        case BRD_HEAP_STRING:
                                return strcmp(a->as.string, b->as.heap->as.string);
                        }
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
                case BRD_VAL_HEAP:
                        switch (b->as.heap->htype) {
                        case BRD_HEAP_STRING:
                                return strcmp(
                                        a->as.boolean ? "true" : "false",
                                        b->as.heap->as.string
                                );
                        }
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
                case BRD_VAL_HEAP:
                        switch (b->as.heap->htype) {
                        case BRD_HEAP_STRING:
                                return -(b->as.heap->as.string[0] != '\0');
                        }
                }
                break;
        case BRD_VAL_BUILTIN:
                BARF("can't compare functions");
                break;
        case BRD_VAL_HEAP:
                switch (a->as.heap->htype) {
                case BRD_HEAP_STRING:
                        switch (b->vtype) {
                        case BRD_VAL_NUM:
                                brd_value_coerce_num(a);
                                return (int) (a->as.num - b->as.num);
                        case BRD_VAL_STRING:
                                return strcmp(a->as.heap->as.string, b->as.string);
                        case BRD_VAL_BOOL:
                                /* This is obviously a very good idea */
                                return strcmp(a->as.heap->as.string, b->as.boolean ? "true" : "false");
                        case BRD_VAL_UNIT:
                                return a->as.heap->as.string[0] != '\0';
                        case BRD_VAL_BUILTIN:
                                BARF("can't compare functions");
                                break;
                        case BRD_VAL_HEAP:
                                switch (b->as.heap->htype) {
                                case BRD_HEAP_STRING:
                                        return strcmp(a->as.heap->as.string, b->as.heap->as.string);
                                }
                        }
                        break;
                }
        }
        BARF("what?");
        return -1;
}

void
brd_value_call(struct brd_value *f, struct brd_value *args, size_t num_args, struct brd_value *out)
{
        if (f->vtype == BRD_VAL_BUILTIN) {
                builtin_function[f->as.builtin](args, num_args, out);
        } else {
                BARF("attempted to call a non-callable");
        }
}

void
brd_value_concat(struct brd_value *a, struct brd_value *b)
{
        /* new string put into a */
        int free_a, free_b;
        const char *string_a, *string_b;
        size_t len_a, len_b;
        struct brd_heap_entry *new;
        
        free_a = brd_value_coerce_string(a);
        free_b = brd_value_coerce_string(b);

        string_a = a->vtype == BRD_VAL_STRING ? a->as.string : a->as.heap->as.string;
        string_b = b->vtype == BRD_VAL_STRING ? b->as.string : b->as.heap->as.string;

        len_a = strlen(string_a);
        len_b = strlen(string_b);

        new = malloc(sizeof(*new));
        new->htype = BRD_HEAP_STRING;
        new->as.string = malloc((len_a + len_b + 1) * sizeof(char));
        strcpy(new->as.string, string_a);
        strcat(new->as.string, string_b);

        if (free_a) {
                free(a->as.heap->as.string);
                free(a->as.heap);
        }
        if (free_b) {
                free(b->as.heap->as.string);
                free(b->as.heap);
        }

        a->vtype = BRD_VAL_HEAP;
        a->as.heap = new;
}

static void
_builtin_write(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        out->vtype = BRD_VAL_UNIT;
        for (int i = 0; i < num_args; i++) {
                struct brd_value value = args[i];

                switch (value.vtype) {
                case BRD_VAL_NUM:
                        printf("%Lg", value.as.num);
                        break;
                case BRD_VAL_STRING:
                        printf("%s", value.as.string);
                        break;
                case BRD_VAL_BOOL:
                        printf("%s", value.as.boolean ? "true" : "false");
                        break;
                case BRD_VAL_UNIT:
                        printf("unit");
                        break;
                case BRD_VAL_BUILTIN:
                        printf("%s", builtin_name[value.as.builtin]);
                        break;
                case BRD_VAL_HEAP:
                        switch (value.as.heap->htype) {
                                case BRD_HEAP_STRING:
                                        printf("%s", value.as.heap->as.string);
                        }
                }
        }
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
        size_t len;
        char *cmd;
        int *malloced = malloc(sizeof(int) * num_args);

        for(int i = 0; i < num_args; i++) {
                malloced[i] = brd_value_coerce_string(&args[i]);
        }

        len = 1;
        for (int i = 0; i < num_args; i++) {
                len += strlen(
                        args[i].vtype == BRD_VAL_STRING ? args[i].as.string
                        : args[i].as.heap->as.string
                );
        }
        cmd = malloc(sizeof(char) * len);
        cmd[0] = '\0';
        for (int i = 0; i < num_args; i++) {
                strcat(
                        cmd,
                        args[i].vtype == BRD_VAL_STRING ? args[i].as.string
                        : args[i].as.heap->as.string
                );
        }

        out->vtype = BRD_VAL_NUM;
        out->as.num = system(cmd);

        for (int i = 0; i < num_args; i++) {
                if (malloced[i]) {
                        free(args[i].as.heap->as.string);
                        free(args[i].as.heap);
                }
        }
        free(malloced);
        free(cmd);
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

