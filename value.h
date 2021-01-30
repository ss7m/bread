#ifndef BRD_VALUE_H
#define BRD_VALUE_H

/* 
 * If I were smarter I'd make the bucket grow
 * But for the mean time that feels like a premature optimization
 * NOTE: I tried to do that but the result was a lot slower lol
 */
#define BUCKET_SIZE 32

struct brd_value;
struct brd_value_closure;

enum brd_heap_type {
        BRD_HEAP_STRING,
        BRD_HEAP_LIST,
        BRD_HEAP_CLOSURE,
};

struct brd_value_list {
        size_t length, capacity;
        struct brd_value *items;
};

void brd_value_list_init(struct brd_value_list *list);
void brd_value_list_push(struct brd_value_list *list, struct brd_value *value);
struct brd_value *brd_value_list_get(struct brd_value_list *list, size_t idx);
void brd_value_list_set(struct brd_value_list *list, size_t idx, struct brd_value *value);

struct brd_heap_entry {
        struct brd_heap_entry *next;
        enum brd_heap_type htype;
        
        /* GC stuff */
        int marked;

        union {
                char *string;
                struct brd_value_list *list;
                struct brd_value_closure *closure;
        } as;
};

void brd_heap_mark(struct brd_heap_entry *entry);

enum brd_value_type {
        BRD_VAL_NUM,
        BRD_VAL_STRING, /* string constant */
        BRD_VAL_BOOL,
        BRD_VAL_UNIT,
        BRD_VAL_BUILTIN,
        BRD_VAL_HEAP,
};

struct brd_value {
        union {
                long double num;
                const char *string;
                int boolean;
                int builtin;
                struct brd_heap_entry *heap;
        } as;
        enum brd_value_type vtype;
};

struct brd_value_map_list {
        char *key;
        struct brd_value_map_list *next;
        struct brd_value val;
};

struct brd_value_map {
        struct brd_value_map_list *bucket;
};

void brd_value_map_init(struct brd_value_map *map);
void brd_value_map_destroy(struct brd_value_map *map);
void brd_value_map_set(struct brd_value_map *map, char *key, struct brd_value *val);
struct brd_value *brd_value_map_get(struct brd_value_map *map, char *key);
void brd_value_map_copy(struct brd_value_map *dest, struct brd_value_map *src);
void brd_value_map_mark(struct brd_value_map *map);

struct brd_value_closure {
        struct brd_value_map env;
        char **args;
        size_t num_args;
        size_t pc;
};

void brd_value_closure_init(struct brd_value_closure *closure, char **args, size_t num_args, size_t pc);
void brd_value_closure_destroy(struct brd_value_closure *closure);

#ifdef DEBUG
void brd_value_debug(struct brd_value *value);
#endif

int brd_value_is_string(struct brd_value *value);
void brd_value_coerce_num(struct brd_value *value);
int brd_value_coerce_string(struct brd_value *value);
int brd_value_index(struct brd_value *value, size_t idx);
int brd_value_truthify(struct brd_value *value);
int brd_value_compare(struct brd_value *a, struct brd_value *b);
void brd_value_concat(struct brd_value *a, struct brd_value *b);

enum brd_builtin {
        BRD_BUILTIN_WRITE,
        BRD_BUILTIN_WRITELN,
        BRD_BUILTIN_READLN,
        BRD_BUILTIN_LENGTH,
        BRD_BUILTIN_TYPEOF,
        BRD_BUILTIN_SYSTEM,
        BRD_NUM_BUILTIN,
};

typedef int (*builtin_fn_dec)(
        struct brd_value *args,
        size_t num_args,
        struct brd_value *out
);

extern const builtin_fn_dec builtin_function[BRD_NUM_BUILTIN];

extern const char *builtin_name[BRD_NUM_BUILTIN];

enum brd_builtin brd_lookup_builtin(char *builtin);

#endif
