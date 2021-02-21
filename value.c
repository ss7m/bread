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

static void
brd_value_list_init_with_capacity(struct brd_value_list *list, size_t capacity)
{
        list->capacity = capacity;
        list->length = 0;
        list->items = malloc(sizeof(struct brd_value) * list->capacity);
}

void
brd_value_list_init(struct brd_value_list *list)
{
        brd_value_list_init_with_capacity(list, 4);
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

void
brd_value_list_set(struct brd_value_list *list, size_t idx, struct brd_value *value)
{
        if (list->length == 0) {
                brd_value_list_push(list, value);
        } else {
                idx = idx % list->length;
                list->items[idx] = *value;
        }
}

char *
brd_value_list_to_string(struct brd_value_list *list) {
        size_t length;
        char *string;
        struct brd_value *list_strings = malloc(sizeof(*list_strings) * list->length);

        length = 4; // "[ ]" and null byte

        for (size_t i = 0; i < list->length; i++) {
                list_strings[i] = list->items[i];
                brd_value_coerce_string(&list_strings[i]);
                length += 2 + AS_STRING(list_strings[i])->length;
        }

        string = malloc(length);
        length = 0;
        string[length++] = '[';
        string[length++] = ' ';

        for (size_t i = 0; i < list->length; i++) {
                strcpy(string + length, AS_STRING(list_strings[i])->s);
                length += AS_STRING(list_strings[i])->length;
                if (i < list->length - 1) {
                        string[length++] = ',';
                }
                string[length++] = ' ';
                if (IS_VAL(list_strings[i], BRD_VAL_HEAP)) {
                        brd_heap_destroy(list_strings[i].as.heap);
                }
        }

        string[length++] = ']';
        string[length] = '\0';
        free(list_strings);

        return string;
}

int
brd_value_list_equals(struct brd_value_list *a, struct brd_value_list *b)
{
        struct brd_comparison cmp;
        if (a->length != b->length) {
                return false;
        }

        for (size_t i = 0; i < a->length; i++) {
                cmp = brd_value_compare(&a->items[i], &b->items[i]);
                if (!brd_comparison_eq(cmp)) {
                        return false;
                }
        }
        return true;
}

void
brd_value_string_init(struct brd_value_string *string, char *s)
{
        string->s = s;
        string->length = strlen(s);
}

struct brd_value_string *
brd_value_string_new(char *s)
{
        struct brd_value_string *string = malloc(sizeof(*string));
        brd_value_string_init(string, s);
        return string;
}

struct brd_heap_entry *
brd_heap_new(enum brd_heap_type htype) {
        struct brd_heap_entry *heap = malloc(sizeof(*heap));
        heap->htype = htype;
        
        switch (htype) {
        case BRD_HEAP_STRING:
                heap->as.string = malloc(sizeof(struct brd_value_string));
                break;
        case BRD_HEAP_LIST:
                heap->as.list = malloc(sizeof(struct brd_value_list));
                break;
        case BRD_HEAP_CLOSURE:
                heap->as.closure = malloc(sizeof(struct brd_value_closure));
                break;
        case BRD_HEAP_CLASS:
                heap->as.class = malloc(sizeof(struct brd_value_class));
                break;
        case BRD_HEAP_OBJECT:
                heap->as.class = malloc(sizeof(struct brd_value_object));
                break;
        }

        return heap;
}

void brd_heap_destroy(struct brd_heap_entry *entry)
{
        switch(entry->htype) {
        case BRD_HEAP_STRING:
                free(entry->as.string->s);
                free(entry->as.string);
                break;
        case BRD_HEAP_LIST:
                free(entry->as.list->items);
                free(entry->as.list);
                break;
        case BRD_HEAP_CLOSURE:
                brd_value_closure_destroy(entry->as.closure);
                free(entry->as.closure);
                break;
        case BRD_HEAP_CLASS:
                brd_value_class_destroy(entry->as.class);
                free(entry->as.class);
                break;
        case BRD_HEAP_OBJECT:
                if (!entry->as.object->is_super) {
                        brd_value_object_destroy(entry->as.object);
                }
                free(entry->as.object);
                break;
        }
        free(entry);
}

void
brd_value_gc_mark(struct brd_value *value)
{
        struct brd_value v;
        if (value->vtype == BRD_VAL_HEAP) {
                struct brd_heap_entry *entry = value->as.heap;
                if (entry->marked) {
                        return;
                }

                entry->marked = true;
                switch (entry->htype) {
                case BRD_HEAP_STRING:
                        break;
                case BRD_HEAP_LIST:
                        for (size_t i = 0; i < entry->as.list->length; i++) {
                                brd_value_gc_mark(&entry->as.list->items[i]);
                        }
                        break;
                case BRD_HEAP_CLOSURE:
                        brd_value_map_mark(&entry->as.closure->env);
                        break;
                case BRD_HEAP_CLASS:
                        /* @Object is marked before GCing, so this is fine */
                        v = brd_heap_value(closure, entry->as.class->constructor);
                        brd_value_gc_mark(&v);
                        v = brd_heap_value(class, entry->as.class->super);
                        brd_value_gc_mark(&v);
                        brd_value_map_mark(&entry->as.class->methods);
                        break;
                case BRD_HEAP_OBJECT:
                        v = brd_heap_value(class, entry->as.object->class);
                        brd_value_gc_mark(&v);
                        brd_value_map_mark(&entry->as.object->fields);
                        break;
                }
        } else if (value->vtype == BRD_VAL_METHOD) {
                v = brd_heap_value(object, value->as.method.this);
                brd_value_gc_mark(&v);
                v = brd_heap_value(closure, value->as.method.fn);
                brd_value_gc_mark(&v);
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
                        brd_value_gc_mark(&p->val);
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
        for (size_t i = 0; i < num_args; i++) {
                brd_value_map_set(&closure->env, args[i], &val);
        }
}

void
brd_value_closure_destroy(struct brd_value_closure *closure)
{
        brd_value_map_destroy(&closure->env);
        free(closure->args);
}

void
brd_value_class_init(struct brd_value_class *class)
{
        brd_value_map_init(&class->methods);
}

void
brd_value_class_subclass(
        struct brd_value_class *sub,
        struct brd_value_class **super,
        struct brd_value_closure **constructor)
{
        sub->super = super;
        sub->constructor = constructor;
        brd_value_class_init(sub);
        brd_value_map_copy(&sub->methods, &(**super).methods);
}

void
brd_value_class_destroy(struct brd_value_class *class)
{
        brd_value_map_destroy(&class->methods);
}

void
brd_value_object_init(struct brd_value_object *object, struct brd_value_class **class)
{
        object->class = class;
        brd_value_map_init(&object->fields);
        object->is_super = false;
}

void
brd_value_object_super(struct brd_value_object *this, struct brd_value_object *super)
{
        super->class = (**this->class).super;
        super->fields = this->fields;
        super->is_super = true;
}

void
brd_value_object_destroy(struct brd_value_object *object)
{
        brd_value_map_destroy(&object->fields);
}

int
brd_comparison_eq(struct brd_comparison cmp)
{
        return cmp.is_ord ? cmp.cmp == 0 : cmp.cmp;
}

void
brd_value_debug(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                printf("%.10Lg", value->as.num);
                break;
        case BRD_VAL_STRING:
                printf("\"%s\"", value->as.string->s);
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
        case BRD_VAL_METHOD:
                printf("<< method >>");
                break;
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        printf("\"%s\"", value->as.heap->as.string->s);
                        break;
                case BRD_HEAP_LIST:
                        printf("[ ");
                        for (size_t i = 0; i < value->as.heap->as.list->length; i++) {
                                brd_value_debug(
                                        &value->as.heap->as.list->items[i]
                                );
                                if (i < value->as.heap->as.list->length - 1) {
                                        printf(",");
                                }
                                printf(" ");
                        }
                        printf("]");
                        break;
                case BRD_HEAP_CLOSURE:
                        printf("<< closure >>");
                        break;
                case BRD_HEAP_CLASS:
                        printf("<< class >>");
                        break;
                case BRD_HEAP_OBJECT:
                        printf("<< object >>");
                        break;
                }
        }
}

int brd_value_is_string(struct brd_value *value)
{
        return IS_VAL(*value, BRD_VAL_STRING) || IS_HEAP(*value, BRD_HEAP_STRING);
}

void
brd_value_coerce_num(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                break;
        case BRD_VAL_STRING:
                value->as.num = strtold(value->as.string->s, NULL);
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
        case BRD_VAL_METHOD:
                BARF("can't coerce a method into a number");
                break;
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        value->as.num = strtold(value->as.heap->as.string->s, NULL);
                        break;
                case BRD_HEAP_LIST:
                        BARF("can't coerce a list to a number");
                        break;
                case BRD_HEAP_CLOSURE:
                        BARF("can't coerce a closure to a number");
                        break;
                case BRD_HEAP_CLASS:
                        BARF("can't coerce a class into a number");
                        break;
                case BRD_HEAP_OBJECT:
                        BARF("can't coerce an object into a number");
                        break;
                }
                break;
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
                value->as.heap = brd_heap_new(BRD_HEAP_STRING);
                brd_value_string_init(value->as.heap->as.string, string);
                return true;
        case BRD_VAL_STRING:
                return false;
        case BRD_VAL_BOOL:
                value->vtype = BRD_VAL_STRING;
                value->as.string = value->as.boolean ? &true_string : &false_string;
                return false;
        case BRD_VAL_UNIT:
                value->vtype = BRD_VAL_STRING;
                value->as.string = &unit_string;
                return false;
        case BRD_VAL_BUILTIN:
                value->vtype = BRD_VAL_STRING;
                value->as.string = &builtin_string[value->as.builtin];
                return false;
        case BRD_VAL_METHOD:
                value->vtype = BRD_VAL_STRING;
                value->as.string = &method_string;
                return false;
        case BRD_VAL_HEAP:
                switch (value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        return false;
                case BRD_HEAP_LIST:
                        string = brd_value_list_to_string(value->as.heap->as.list);
                        value->vtype = BRD_VAL_HEAP;
                        value->as.heap = brd_heap_new(BRD_HEAP_STRING);
                        brd_value_string_init(value->as.heap->as.string, string);
                        return true;
                case BRD_HEAP_CLOSURE:
                        value->vtype = BRD_VAL_STRING;
                        value->as.string = &closure_string;
                        return false;
                case BRD_HEAP_CLASS:
                        value->vtype = BRD_VAL_STRING;
                        value->as.string = &class_string;
                        return false;
                case BRD_HEAP_OBJECT:
                        value->vtype = BRD_VAL_STRING;
                        value->as.string = &object_string;
                        return false;
                }
                break;
        }
        BARF("what?");
        return -1;
}

static intmax_t
brd_value_index_clamp(intmax_t idx, size_t length)
{
        if (length == 0) {
                return idx;
        } else if (idx >= 0) {
                return idx % length;
        } else {
                return length - (-idx) % length;
        }
}

int
brd_value_index(struct brd_value *value, intmax_t idx)
{
        if IS_STRING(*value) {
                struct brd_value_string *string = AS_STRING(*value);
                char *c;

                idx = brd_value_index_clamp(idx, string->length);
                if (string->length == 0) {
                        value->vtype = BRD_VAL_UNIT;
                        return false;
                }

                c = malloc(2);
                c[0] = string->s[idx];
                c[1] = '\0';
                value->vtype = BRD_VAL_HEAP;
                value->as.heap = brd_heap_new(BRD_HEAP_STRING);
                brd_value_string_init(value->as.heap->as.string, c);
                return true;
        } else if (IS_HEAP(*value, BRD_HEAP_LIST)) {
                struct brd_value_list *list = value->as.heap->as.list;
                idx = brd_value_index_clamp(idx, list->length);
                if (list->length == 0) {
                        value->vtype = BRD_VAL_UNIT;
                        return false;
                } else {
                        *value = list->items[idx];
                        return false;
                }
        } else {
                BARF("attempted to index a non-indexable");
                return -1;
        }
}

int
brd_value_truthify(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                return value->as.num != 0;
        case BRD_VAL_STRING:
                return value->as.string->length > 0;
        case BRD_VAL_BOOL:
                return value->as.boolean;
        case BRD_VAL_UNIT:
                return false;
        case BRD_VAL_BUILTIN:
                return true;
        case BRD_VAL_METHOD:
                return true;
        case BRD_VAL_HEAP:
                switch(value->as.heap->htype) {
                case BRD_HEAP_STRING:
                        return value->as.heap->as.string->length > 0;
                case BRD_HEAP_LIST:
                        return value->as.heap->as.list->length > 0;
                case BRD_HEAP_CLOSURE:
                        return true;
                case BRD_HEAP_CLASS:
                        return true;
                case BRD_HEAP_OBJECT:
                        return true;
                }
        }
        BARF("what?");
        return false;
}

// https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
#define signum(a) ((0 < (a)) - ((a) < 0)) //why isn't this in math.h?
struct brd_comparison
brd_value_compare(struct brd_value *a, struct brd_value *b)
{
        struct brd_comparison result;

        if (IS_STRING(*a)) {
                char *sa = AS_STRING(*a)->s;
                if (IS_STRING(*b)) {
                        result.cmp = signum(strcmp(sa, AS_STRING(*b)->s));
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_NUM)) {
                        long double da = strtold(sa, NULL);
                        result.cmp = signum(da - b->as.num);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_BOOL)) {
                        result.cmp = strcmp(sa, b->as.boolean ? "true" : "false");
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_UNIT)) {
                        result.cmp = signum(strcmp(sa, "unit"));
                        result.is_ord = true;
                } else {
                        result.cmp = false;
                        result.is_ord = false;
                }
        } else if (IS_VAL(*a, BRD_VAL_NUM)) {
                long double da = a->as.num;
                if (IS_STRING(*b)) {
                        long double db = strtold(AS_STRING(*b)->s, NULL);
                        result.cmp = signum(da - db);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_NUM)) {
                        result.cmp = signum(da - b->as.num);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_BOOL)) {
                        result.cmp = signum(da - b->as.boolean);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_UNIT)) {
                        result.cmp = signum(da);
                        result.is_ord = true;
                } else {
                        result.cmp = false;
                        result.is_ord = false;
                }
        } else if (IS_VAL(*a, BRD_VAL_BOOL)) {
                int ba = a->as.boolean;
                if (IS_STRING(*b)) {
                        result.cmp = strcmp(ba ? "true" : "false", AS_STRING(*b)->s);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_NUM)) {
                        result.cmp = signum(ba - b->as.num);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_BOOL)) {
                        result.cmp = ba == b->as.boolean;
                        result.is_ord = false;
                } else if (IS_VAL(*b, BRD_VAL_UNIT)) {
                        result.cmp = ba;
                        result.is_ord = false;
                } else {
                        result.cmp = false;
                        result.is_ord = false;
                }
        } else if (IS_VAL(*a, BRD_VAL_UNIT)) {
                if (IS_STRING(*b)) {
                        result.cmp = strcmp("unit", AS_STRING(*b)->s);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_NUM)) {
                        result.cmp = signum(-b->as.num);
                        result.is_ord = true;
                } else if (IS_VAL(*b, BRD_VAL_BOOL)) {
                        result.cmp = b->as.boolean;
                        result.is_ord = false;
                } else if (IS_VAL(*b, BRD_VAL_UNIT)) {
                        result.cmp = 0;
                        result.is_ord = true;
                } else {
                        result.cmp = false;
                        result.is_ord = false;
                }
        } else if (IS_VAL(*a, BRD_VAL_METHOD)) {
                struct brd_value_method ma = a->as.method;
                if (IS_VAL(*b, BRD_VAL_METHOD)) {
                        struct brd_value_method mb = b->as.method;
                        result.cmp = ma.this == mb.this && ma.fn == mb.fn;
                        result.is_ord = false;
                } else {
                        result.cmp = false;
                        result.is_ord = false;
                }
        } else if (IS_HEAP(*a, BRD_HEAP_LIST)) {
                struct brd_value_list *la = a->as.heap->as.list;
                if (IS_HEAP(*b, BRD_HEAP_LIST)) {
                        struct brd_value_list *lb = b->as.heap->as.list;
                        result.cmp = brd_value_list_equals(la, lb);
                        result.is_ord = false;
                } else {
                        result.cmp = false;
                        result.is_ord = false;
                }
        } else if (IS_VAL(*a, BRD_VAL_HEAP)) {
                if (IS_VAL(*b, BRD_VAL_HEAP)) {
                        result.cmp = a->as.heap == b->as.heap;
                        result.is_ord = false;
                } else {
                        result.cmp = false;
                        result.is_ord = false;
                }
        } else {
                BARF("what?");
        }

        return result;
}
#undef signum

void
brd_value_concat(struct brd_value *a, struct brd_value *b)
{
        struct brd_heap_entry *new;
        if (IS_HEAP(*a, BRD_HEAP_LIST)) {
                struct brd_value_list *list_a = a->as.heap->as.list;

                new = brd_heap_new(BRD_HEAP_LIST);
                if (IS_HEAP(*b, BRD_HEAP_LIST)) {
                        struct brd_value_list *list_b = b->as.heap->as.list;

                        brd_value_list_init_with_capacity(
                                new->as.list, list_a->length + list_b->length
                        );
                        new->as.list->length = list_a->length + list_b->length;

                        for (size_t i = 0; i < list_a->length; i++) {
                                new->as.list->items[i] = list_a->items[i];
                        }
                        for (size_t i = 0; i < list_b->length; i++) {
                                new->as.list->items[i + list_a->length]
                                        = list_b->items[i];
                        }
                } else {
                        brd_value_list_init_with_capacity(
                                new->as.list, list_a->length + 1
                        );
                        new->as.list->length = list_a->length + 1;
                        
                        for (size_t i = 0; i < list_a->length; i++) {
                                new->as.list->items[i] = list_a->items[i];
                        }
                        new->as.list->items[list_a->length] = *b;
                }
        } else {
                int free_a, free_b;
                char *new_string;

                free_a = brd_value_coerce_string(a);
                free_b = brd_value_coerce_string(b);
                new = brd_heap_new(BRD_HEAP_STRING);
                
                new_string = malloc(AS_STRING(*a)->length + AS_STRING(*b)->length + 1);
                strcpy(new_string, AS_STRING(*a)->s);
                strcpy(new_string + AS_STRING(*a)->length, AS_STRING(*b)->s);
                brd_value_string_init(new->as.string, new_string);

                if (free_a) {
                        brd_heap_destroy(a->as.heap);
                }
                if (free_b) {
                        brd_heap_destroy(b->as.heap);
                }
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

        for (size_t i = 0; i < num_args; i++) {
                struct brd_value value = args[i];

                switch (value.vtype) {
                case BRD_VAL_NUM:
                        printf("%.10Lg", value.as.num);
                        break;
                case BRD_VAL_STRING:
                        printf("%s", value.as.string->s);
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
                case BRD_VAL_METHOD:
                        printf("<< method >>");
                        break;
                case BRD_VAL_HEAP:
                        switch (value.as.heap->htype) {
                        case BRD_HEAP_STRING:
                                printf("%s", value.as.heap->as.string->s);
                                break;
                        case BRD_HEAP_LIST:
                                printf("[ ");
                                for (size_t j = 0; j < value.as.heap->as.list->length; j++) {
                                        _builtin_write(
                                                &value.as.heap->as.list->items[j],
                                                1,
                                                NULL
                                        );
                                        if (j < value.as.heap->as.list->length - 1) {
                                                printf(",");
                                        }
                                        printf(" ");
                                }
                                printf("]");
                                break;
                        case BRD_HEAP_CLOSURE:
                                printf("<< closure >>");
                                break;
                        case BRD_HEAP_CLASS:
                                printf("<< class >>");
                                break;
                        case BRD_HEAP_OBJECT:
                                printf("<< object >>");
                                break;
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
        char *str;

        (void)args;
        if (num_args > 0) {
                BARF("readln accepts no arguments");
        }

        out->vtype = BRD_VAL_HEAP;
        out->as.heap = brd_heap_new(BRD_HEAP_STRING);

        str = NULL;
        n = getline(&str, &n, stdin);
        str[n-1] = '\0'; /* remove newline */
        brd_value_string_init(out->as.heap->as.string, str);
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
                out->as.num = args[0].as.string->length;
                break;
        case BRD_VAL_HEAP:
                switch (args[0].as.heap->htype) {
                case BRD_HEAP_STRING:
                        out->as.num = args[0].as.heap->as.string->length;
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
                out->as.string = &number_string;
                break;
        case BRD_VAL_STRING:
                out->as.string = &string_string;
                break;
        case BRD_VAL_BOOL:
                out->as.string = &boolean_string;
                break;
        case BRD_VAL_UNIT:
                out->as.string = &unit_string;
                break;
        case BRD_VAL_BUILTIN:
                out->as.string = &builtin_string[args[0].as.builtin];
                break;
        case BRD_VAL_METHOD:
                out->as.string = &method_string;
                break;
        case BRD_VAL_HEAP:
                switch (args[0].as.heap->htype) {
                case BRD_HEAP_STRING:
                        out->as.string = &string_string;
                        break;
                case BRD_HEAP_LIST:
                        out->as.string = &list_string;
                        break;
                case BRD_HEAP_CLOSURE:
                        out->as.string = &closure_string;
                        break;
                case BRD_HEAP_CLASS:
                        out->as.string = &class_string;
                        break;
                case BRD_HEAP_OBJECT:
                        out->as.string = &object_string;
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

        for(size_t i = 0; i < num_args; i++) {
                malloced[i] = brd_value_coerce_string(&args[i]);
        }

        len = 1;
        for (size_t i = 0; i < num_args; i++) {
                len += AS_STRING(args[i])->length;
        }
        cmd = malloc(sizeof(char) * len);
        cmd[0] = '\0';
        for (size_t i = 0; i < num_args; i++) {
                strcat(cmd, AS_STRING(args[i])->s);
        }

        out->vtype = BRD_VAL_NUM;
        out->as.num = system(cmd);

        for (size_t i = 0; i < num_args; i++) {
                if (malloced[i]) {
                        brd_heap_destroy(args[i].as.heap);
                }
        }
        free(malloced);
        free(cmd);
        return false;
}

static int
_builtin_string(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        if (num_args != 1) {
                BARF("@string accepts exactly one argument");
        }

        *out = args[0];
        return brd_value_coerce_string(out);
}

static int
_builtin_push(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        struct brd_value_list *list;
        if (num_args == 0) {
                BARF("@push should be given at least one argument");
        } else if (!IS_HEAP(args[0], BRD_HEAP_LIST)) {
                BARF("first argument to @push should be a list");
        }

        list = args[0].as.heap->as.list;
        for (size_t i = 1; i < num_args; i++) {
                brd_value_list_push(list, &args[i]);
        }

        out->vtype = BRD_VAL_UNIT;
        return false;
}

static int
_builtin_insert(struct brd_value *args, size_t num_args, struct brd_value *out)
{
        struct brd_value_list *list;
        size_t idx;
        long double num;

        if (num_args != 3) {
                BARF("@insert accepts exactly 3 arguments");
        } else if (!IS_HEAP(args[0], BRD_HEAP_LIST)) {
                BARF("first argument to @insert should be a list");
        }

        list = args[0].as.heap->as.list;

        brd_value_coerce_num(&args[2]);
        num = floorl(args[2].as.num);
        if (num < 0) {
                BARF("index for @insert cannot be negative");
        }
        idx = num;

        if (list->length <= idx) {
                struct brd_value unit;
                unit.vtype = BRD_VAL_UNIT;

                for (size_t i = list->length; i < idx; i++) {
                        brd_value_list_push(list, &unit);
                }
                brd_value_list_push(list, &args[1]);
        } else {
                struct brd_value_list new;
                brd_value_list_init_with_capacity(&new, list->length + 1);

                for (size_t i = 0; i < idx; i++) {
                        new.items[i] = list->items[i];
                }
                new.items[idx] = args[1];
                for (size_t i = idx + 1; i < list->length + 1; i++) {
                        new.items[i] = list->items[i - 1];
                }

                new.length = list->length + 1;
                free(list->items);
                *list = new;
        }

        out->vtype = BRD_VAL_UNIT;
        return false;
}

enum brd_builtin brd_lookup_builtin(char *builtin)
{
        for (int i = 0; i < BRD_NUM_BUILTIN; i++) {
                if (strcmp(builtin_name[i], builtin) == 0) {
                        return i;
                }
        }

        if (strcmp("Object", builtin) == 0) {
                return BRD_GLOBAL_OBJECT;
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
        [BRD_BUILTIN_STRING] = _builtin_string,
        [BRD_BUILTIN_PUSH] = _builtin_push,
        [BRD_BUILTIN_INSERT] = _builtin_insert,
};

const char *builtin_name[BRD_NUM_BUILTIN] = {
        [BRD_BUILTIN_WRITE] = "write",
        [BRD_BUILTIN_WRITELN] = "writeln",
        [BRD_BUILTIN_READLN] = "readln",
        [BRD_BUILTIN_LENGTH] = "length",
        [BRD_BUILTIN_TYPEOF] = "typeof",
        [BRD_BUILTIN_SYSTEM] = "system",
        [BRD_BUILTIN_STRING] = "string",
        [BRD_BUILTIN_PUSH] = "push",
        [BRD_BUILTIN_INSERT] = "insert",
};

#define MK_BUILTIN_STRING(str) { str, sizeof(str) - 1 }

struct brd_value_string builtin_string[BRD_NUM_BUILTIN] = {
        [BRD_BUILTIN_WRITE]   = MK_BUILTIN_STRING("@write"),
        [BRD_BUILTIN_WRITELN] = MK_BUILTIN_STRING("@writeln"),
        [BRD_BUILTIN_READLN]  = MK_BUILTIN_STRING("@readln"),
        [BRD_BUILTIN_LENGTH]  = MK_BUILTIN_STRING("@length"),
        [BRD_BUILTIN_TYPEOF]  = MK_BUILTIN_STRING("@typeof"),
        [BRD_BUILTIN_SYSTEM]  = MK_BUILTIN_STRING("@system"),
        [BRD_BUILTIN_STRING]  = MK_BUILTIN_STRING("@string"),
        [BRD_BUILTIN_PUSH] = MK_BUILTIN_STRING("@push"),
        [BRD_BUILTIN_INSERT] = MK_BUILTIN_STRING("@insert"),
};

struct brd_value_string number_string = MK_BUILTIN_STRING("number");
struct brd_value_string string_string = MK_BUILTIN_STRING("string");
struct brd_value_string boolean_string = MK_BUILTIN_STRING("boolean");
struct brd_value_string unit_string = MK_BUILTIN_STRING("unit");
struct brd_value_string method_string = MK_BUILTIN_STRING("method");
struct brd_value_string list_string = MK_BUILTIN_STRING("list");
struct brd_value_string closure_string = MK_BUILTIN_STRING("closure");
struct brd_value_string class_string = MK_BUILTIN_STRING("class");
struct brd_value_string object_string = MK_BUILTIN_STRING("object");
struct brd_value_string true_string = MK_BUILTIN_STRING("true");
struct brd_value_string false_string = MK_BUILTIN_STRING("false");

struct brd_value object_class;

#undef MK_BUILTIN_STRING
