#include "common.h"
#include "value.h"

#include <math.h>

/* http://www.cse.yorku.ca/~oz/hash.html djb2 hash algorithm */
static unsigned long
hash(char *str)
{
        unsigned long hash = 5381;
        int c;

        while ((c = *str++))
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
}

void
brd_value_list_init(struct brd_value_list *list)
{
        list->capacity = 4;
        list->length = 0;
        list->items = malloc(sizeof(struct brd_value) * list->capacity);
}

void
brd_value_list_push(struct brd_value_list *list, struct brd_value *value)
{
        if (list->capacity == list->length) {
                list->capacity *= 1.5;
                list->items = realloc(
                        list->items,
                        sizeof(struct brd_value) * list->capacity
                );
        }

        list->items[list->length++] = *value;
}

struct brd_value *
brd_value_list_get(struct brd_value_list *list, size_t idx)
{
        idx = idx % list->length;
        return &list->items[idx];
}

void
brd_value_list_set(struct brd_value_list *list, size_t idx, struct brd_value *value)
{
        idx = idx % list->length;
        list->items[idx] = *value;
}

void
brd_value_string_init(struct brd_value_string *string, char *s)
{
        string->s = s;
        string->length = strlen(s);
}

void
brd_heap_mark(struct brd_heap_entry *entry)
{
        if (entry->marked) {
                return;
        }

        entry->marked = true;
        switch (entry->htype) {
        case BRD_HEAP_STRING:
                break;
        case BRD_HEAP_LIST:
                for (int i = 0; i < entry->as.list->length; i++) {
                        if (entry->as.list->items[i].vtype == BRD_VAL_HEAP) {
                                brd_heap_mark(entry->as.list->items[i].as.heap);
                        }
                }
                break;
        case BRD_HEAP_CLOSURE:
                brd_value_map_mark(&entry->as.closure->env);
                break;
        }
}

void
brd_value_map_init(struct brd_value_map *map)
{
        map->bucket = malloc(sizeof(struct brd_value_map_list) * BUCKET_SIZE);
        for (int i = 0; i < BUCKET_SIZE; i++) {
                map->bucket[i].key = "";
                map->bucket[i].val.vtype = BRD_VAL_UNIT;
                map->bucket[i].next = NULL;
        }
}

void
brd_value_map_destroy(struct brd_value_map *map)
{
        /* At this point in time I'm not going to free the strings */

        for (int i = 0; i < BUCKET_SIZE; i++) {
                struct brd_value_map_list *list = map->bucket[i].next;
                
                while (list != NULL) {
                        struct brd_value_map_list *next = list->next;
                        free(list);
                        list = next;
                }
        }
        free(map->bucket);
}

void
brd_value_map_set(struct brd_value_map *map, char *key, struct brd_value *val)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct brd_value_map_list *list = &map->bucket[h];

        /*
         * Always at least 1 iteration since each bucket entry
         * has an initial dummy value
         */
        for (;;) {
                if (strcmp(list->key, key) == 0) {
                        list->val = *val;
                        return;
                } else if (list->next == NULL) {
                        break;
                } else {
                        list = list->next;
                }
        }

        /* key not in list */
        list->next = malloc(sizeof(struct brd_value_map_list));
        list->next->key = key;
        list->next->val = *val;
        list->next->next = NULL;
}

struct brd_value *
brd_value_map_get(struct brd_value_map *map, char *key)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct brd_value_map_list *list = &map->bucket[h];

        do {
                if (strcmp(list->key, key) == 0) {
                        return &list->val;
                }
        } while ((list = list->next) != NULL);

        return NULL;
}

void
brd_value_map_copy(struct brd_value_map *dest, struct brd_value_map *src)
{
        for (int i = 0; i < BUCKET_SIZE; i++) {
                struct brd_value_map_list *entry = src->bucket[i].next;
                while (entry != NULL) {
                        brd_value_map_set(dest, entry->key, &entry->val);
                        entry = entry->next;
                }
        }
}

void
brd_value_map_mark(struct brd_value_map *map)
{
        for (int i = 0; i < BUCKET_SIZE; i++) {
                struct brd_value_map_list *p = &map->bucket[i];
                while (p != NULL) {
                        if (p->val.vtype == BRD_VAL_HEAP) {
                                brd_heap_mark(p->val.as.heap);
                        }
                        p = p->next;
                }
        }
}

void
brd_value_closure_init(struct brd_value_closure *closure, char **args, size_t num_args, size_t pc)
{
        struct brd_value val;

        brd_value_map_init(&closure->env);
        closure->args = args;
        closure->num_args = num_args;
        closure->pc = pc;

        /* 
         * technically this isn't necessary, but theoretically
         * with a large env this will make accessing function arguments
         * more efficient
         */
        val.vtype = BRD_VAL_UNIT;
        for (int i = 0; i < num_args; i++) {
                brd_value_map_set(&closure->env, args[i], &val);
        }
}

void
brd_value_closure_destroy(struct brd_value_closure *closure)
{
        brd_value_map_destroy(&closure->env);
        free(closure->args);
}

#ifdef DEBUG
void
brd_value_debug(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                printf("%.10Lg", value->as.num);
                break;
        case BRD_VAL_STRING:
                printf("\"%s\"", value->as.string);
                break;
        case BRD_VAL_BOOL:
                printf("%s", value->as.boolean ? "true" : "false");
                break;
        case BRD_VAL_UNIT:
                printf("unit");
                break;
        case BRD_VAL_BUILTIN:
                printf("<< builtin: %s >>", builtin_name[value->as.builtin]);
                break;
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        printf("%s", value->as.heap->as.string);
                        break;
                case BRD_HEAP_LIST:
                        printf("[ ");
                        for (int i = 0; i < value->as.heap->as.list->length; i++) {
                                brd_value_debug(
                                        &value->as.heap->as.list->items[i]
                                );
                                if (i < value->as.heap->as.list->length - 1) {
                                        printf(", ");
                                }
                        }
                        printf(" ]");
                        break;
                case BRD_HEAP_CLOSURE:
                        printf("closure");
                        break;
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
                        break;
                case BRD_HEAP_LIST:
                        BARF("can't coerce a list to a number");
                        break;
                case BRD_HEAP_CLOSURE:
                        BARF("can't coerce a closure to a number");
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
                case BRD_HEAP_LIST:
                        BARF("write this");
                        return true;
                case BRD_HEAP_CLOSURE:
                        BARF("can't convert a closure to a string");
                        return -1;
                }
        }
        BARF("what?");
        return -1;
}

int
brd_value_index(struct brd_value *value, size_t idx)
{
        const char *string;
        char *c;

        switch (value->vtype) {
        case BRD_VAL_STRING:
                string = value->as.string;
                goto index_string;
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        string = value->as.heap->as.string;
                        goto index_string;
                case BRD_HEAP_LIST:
                        *value = *brd_value_list_get(value->as.heap->as.list, idx);
                        return false;
                default:
                        BARF("attempted to index a non-indexable");
                        return -1;
                }
        default:
                BARF("attempted to index a non-indexable");
                return -1;
        }

index_string:
        idx = idx % strlen(string);
        c = malloc(sizeof(char) * 2);
        c[0] = string[idx];
        c[1] = '\0';
        value->vtype = BRD_VAL_HEAP;
        value->as.heap = malloc(sizeof(struct brd_heap_entry));
        value->as.heap->htype = BRD_HEAP_STRING;
        value->as.heap->as.string = c;
        return true;
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
                case BRD_HEAP_LIST:
                        return value->as.heap->as.list->length > 0;
                case BRD_HEAP_CLOSURE:
                        return true;
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
                        case BRD_HEAP_LIST:
                                BARF("can't compare a list and a string");
                                return -1;
                        case BRD_HEAP_CLOSURE:
                                BARF("can't compare a closure and a string");
                                return -1;
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
                        case BRD_HEAP_LIST:
                                BARF("can't compare a list and a boolean");
                                return -1;
                        case BRD_HEAP_CLOSURE:
                                BARF("can't compare a closure and a boolean");
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
                        case BRD_HEAP_LIST:
                                BARF("can't compare a list and unit");
                                return -1;
                        case BRD_HEAP_CLOSURE:
                                BARF("can't compare a closure and a unit");
                                return -1;
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
                                case BRD_HEAP_LIST:
                                        BARF("can't compare a list and a string");
                                        return -1;
                                case BRD_HEAP_CLOSURE:
                                        BARF("can't compare a closure and a string");
                                }
                        }
                        break;
                case BRD_HEAP_LIST:
                        if (b->vtype == BRD_VAL_HEAP
                                        && b->as.heap->htype == BRD_HEAP_LIST) {
                                BARF("write this code");
                                return -1;
                        } else {
                                BARF("can only compare lists with lists");
                                return -1;
                        }
                case BRD_HEAP_CLOSURE:
                        BARF("can't compare closures");
                        return -1;
                }
        }
        BARF("what?");
        return -1;
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

static int
_builtin_write(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        if (out != NULL) {
                out->vtype = BRD_VAL_UNIT;
        }

        for (int i = 0; i < num_args; i++) {
                struct brd_value value = args[i];

                switch (value.vtype) {
                case BRD_VAL_NUM:
                        printf("%.10Lg", value.as.num);
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
                                break;
                        case BRD_HEAP_LIST:
                                printf("[ ");
                                for (int j = 0; j < value.as.heap->as.list->length; j++) {
                                        _builtin_write(
                                                &value.as.heap->as.list->items[j],
                                                1,
                                                NULL
                                        );
                                        if (j < value.as.heap->as.list->length - 1) {
                                                printf(", ");
                                        }
                                }
                                printf(" ]");
                                break;
                        case BRD_HEAP_CLOSURE:
                                printf("closure");
                        }
                }
        }

        return false;
}

static int
_builtin_writeln(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        _builtin_write(args, num_args, out);
        printf("\n");
        return false;
}

static int
_builtin_readln(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        size_t n = 0;

        (void)args;
        if (num_args > 0) {
                BARF("readln accepts no arguments");
        }

        out->vtype = BRD_VAL_HEAP;
        out->as.heap = malloc(sizeof(struct brd_heap_entry));
        out->as.heap->htype = BRD_HEAP_STRING;
        out->as.heap->as.string = NULL;

        n = getline(&out->as.heap->as.string, &n, stdin);
        out->as.heap->as.string[n-1] = '\0'; /* remove newline */
        return true;
}

static int
_builtin_length(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        if (num_args != 1) {
                BARF("length accepts exactly 1 argument");
        }

        out->vtype = BRD_VAL_NUM;

        switch(args[0].vtype) {
        case BRD_VAL_STRING:
                out->as.num = strlen(args[0].as.string);
                break;
        case BRD_VAL_HEAP:
                switch (args[0].as.heap->htype) {
                case BRD_HEAP_STRING:
                        out->as.num = strlen(args[0].as.heap->as.string);
                        break;
                case BRD_HEAP_LIST:
                        out->as.num = args[0].as.heap->as.list->length;
                        break;
                default:
                        BARF("called length on a non-length-haver");
                        break;
                }
                break;
        default:
                BARF("called length on a non-length-haver");
        }

        return false;
}

static int
_builtin_typeof(struct brd_value *args, size_t num_args, struct brd_value *out)
{

        if (num_args != 1) {
                BARF("typeof accepts exactly 1 argument");
        }

        out->vtype = BRD_VAL_STRING;
        switch (args[0].vtype) {
        case BRD_VAL_NUM:
                out->as.string = "number";
                break;
        case BRD_VAL_STRING:
                out->as.string = "string";
                break;
        case BRD_VAL_BOOL:
                out->as.string = "boolean";
                break;
        case BRD_VAL_UNIT:
                out->as.string = "unit";
                break;
        case BRD_VAL_BUILTIN:
                out->as.string = "builtin";
                break;
        case BRD_VAL_HEAP:
                switch (args[0].as.heap->htype) {
                case BRD_HEAP_STRING:
                        out->as.string = "string";
                        break;
                case BRD_HEAP_LIST:
                        out->as.string = "list";
                        break;
                case BRD_HEAP_CLOSURE:
                        out->as.string = "closure";
                        break;
                }
        }

        return false;
}

static int
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
        return false;
}

enum brd_builtin brd_lookup_builtin(char *builtin)
{
        for (int i = 0; i < BRD_NUM_BUILTIN; i++) {
                if (strcmp(builtin_name[i], builtin) == 0) {
                        return i;
                }
        }

        BARFA("Unknown builtin: %s", builtin);
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

