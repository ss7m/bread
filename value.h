#ifndef BRD_VALUE_H
#define BRD_VALUE_H

/*
 * Structs with a double pointer field have that so that we can later
 * retrieve the containing heap for GC purposes
 */

/* 
 * If I were smarter I'd make the bucket grow
 * But for the mean time that feels like a premature optimization
 * NOTE: I tried to do that but the result was a lot slower lol
 */
#define BUCKET_SIZE 24

// inspired by wl_container_of from wayland
#define brd_containing_heap(type, item) ((struct brd_heap_entry *)\
        (((char *)(item)) - offsetof(struct brd_heap_entry, as.type)))

#define brd_containing_value(type, item) ((struct brd_value *)\
        (((char *)(item)) - offsetof(struct brd_value, as.type)))

#define brd_heap_value(type, item) (struct brd_value){\
        .as.heap = brd_containing_heap(type, (item)),\
        .vtype = BRD_VAL_HEAP\
}

#define IS_VAL(v, type) ((v).vtype == type)
#define IS_HEAP(v, type) (IS_VAL(v, BRD_VAL_HEAP) && (v).as.heap->htype == type)
#define IS_STRING(v) (IS_VAL((v), BRD_VAL_STRING) || IS_HEAP((v), BRD_HEAP_STRING))
#define AS_STRING(v) (IS_VAL((v), BRD_VAL_STRING) ? (v).as.string : (v).as.heap->as.string)

struct brd_value;
struct brd_value_closure;
struct brd_value_list;
struct brd_value_map;
struct brd_value_string;
struct brd_value_class;
struct brd_value_object;
struct brd_value_dict;

enum brd_heap_type {
        BRD_HEAP_STRING,
        BRD_HEAP_LIST,
        BRD_HEAP_CLOSURE,
        BRD_HEAP_CLASS,
        BRD_HEAP_OBJECT,
        BRD_HEAP_DICT,
};

struct brd_value_list {
        size_t length, capacity;
        struct brd_value *items;
};

void brd_value_list_init(struct brd_value_list *list);
void brd_value_list_push(struct brd_value_list *list, struct brd_value *value);
void brd_value_list_set(struct brd_value_list *list, size_t idx, struct brd_value *value);
char *brd_value_list_to_string(struct brd_value_list *list);
int brd_value_list_equals(struct brd_value_list *a, struct brd_value_list *b);

struct brd_value_string {
        char *s;
        size_t length;
};

void brd_value_string_init(struct brd_value_string *string, char *s);
struct brd_value_string *brd_value_string_new(char *s);

struct brd_heap_entry {
        struct brd_heap_entry *next;

        union {
                struct brd_value_string *string;
                struct brd_value_list *list;
                struct brd_value_closure *closure;
                struct brd_value_class *class;
                struct brd_value_object *object;
                struct brd_value_dict *dict;
        } as;

        int marked; /* for GC */

        enum brd_heap_type htype;
        char _p[3];
};

struct brd_heap_entry *brd_heap_new(enum brd_heap_type htype);
void brd_heap_destroy(struct brd_heap_entry *entry);

enum brd_value_type {
        BRD_VAL_NUM,
        BRD_VAL_STRING, /* string constant */
        BRD_VAL_BOOL,
        BRD_VAL_UNIT,
        BRD_VAL_BUILTIN,
        BRD_VAL_METHOD,
        BRD_VAL_HEAP,
};

struct brd_value_method {
        struct brd_value_object **this;
        struct brd_value_closure **fn;
};

struct brd_value {
        // can't nan box because sizeof(method) == sizeof(long double)
        union {
                long double num;
                struct brd_value_string *string;
                int boolean;
                int builtin;
                struct brd_value_method method;
                struct brd_heap_entry *heap;
        } as;
        enum brd_value_type vtype;
        char _p[15];
        // not loving that we're wasting 15 bytes here
};

void brd_value_gc_mark(struct brd_value *value);

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

struct brd_value_class {
        struct brd_value_class **super;
        struct brd_value_closure **constructor;
        struct brd_value_map methods;
};

void brd_value_class_init(struct brd_value_class *class);
void brd_value_class_subclass(struct brd_value_class *sub, struct brd_value_class **super, struct brd_value_closure **constructor);
void brd_value_class_destroy(struct brd_value_class *class);

struct brd_value_object {
        struct brd_value_class **class;
        struct brd_value_map fields;
        int is_super;
        char _p[4];
};

void brd_value_object_init(struct brd_value_object *object, struct brd_value_class **class);
void brd_value_object_destroy(struct brd_value_object *object);
void brd_value_object_super(struct brd_value_object *this, struct brd_value_object *super);

struct brd_value_dict {
        struct brd_value_list keys;
        struct brd_value_map map;
        size_t size;
};

void brd_value_dict_init(struct brd_value_dict *dict);
void brd_value_dict_destroy(struct brd_value_dict *dict);
struct brd_value *brd_value_dict_get(struct brd_value_dict *dict, struct brd_value *key);
void brd_value_dict_set(struct brd_value_dict *dict, struct brd_value *key, struct brd_value *value);
char *brd_value_dict_to_string(struct brd_value_dict *dict);

struct brd_comparison {
        signed char cmp;
        char is_ord;
};

int brd_comparison_eq(struct brd_comparison cmp);

#define brd_comparison_ord(cmp, op, out) do {\
        if (cmp.is_ord) {\
                out = cmp.cmp op 0;\
        } else {\
                out = false;\
        }\
} while(0)

void brd_value_debug(struct brd_value *value);
int brd_value_is_string(struct brd_value *value);
void brd_value_coerce_num(struct brd_value *value);
int brd_value_coerce_string(struct brd_value *value);
int brd_value_index(struct brd_value *value, intmax_t idx);
int brd_value_truthify(struct brd_value *value);
struct brd_comparison brd_value_compare(struct brd_value *a, struct brd_value *b);
void brd_value_concat(struct brd_value *a, struct brd_value *b);

enum brd_builtin {
        BRD_BUILTIN_WRITE,
        BRD_BUILTIN_WRITELN,
        BRD_BUILTIN_READLN,
        BRD_BUILTIN_LENGTH,
        BRD_BUILTIN_TYPEOF,
        BRD_BUILTIN_SYSTEM,
        BRD_BUILTIN_STRING,
        BRD_BUILTIN_PUSH,
        BRD_BUILTIN_INSERT,
        BRD_BUILTIN_DICT,
        BRD_BUILTIN_SUBCLASS,
        BRD_NUM_BUILTIN,
        BRD_GLOBAL_OBJECT,
};

typedef int (*builtin_fn_dec)(
        struct brd_value *args,
        size_t num_args,
        struct brd_value *out
);

enum brd_builtin brd_lookup_builtin(char *builtin);

extern const builtin_fn_dec builtin_function[BRD_NUM_BUILTIN];

extern const char *builtin_name[BRD_NUM_BUILTIN];

extern struct brd_value_string builtin_string[BRD_NUM_BUILTIN];
extern struct brd_value_string number_string;
extern struct brd_value_string string_string;
extern struct brd_value_string boolean_string;
extern struct brd_value_string unit_string;
extern struct brd_value_string method_string;
extern struct brd_value_string list_string;
extern struct brd_value_string closure_string;
extern struct brd_value_string class_string;
extern struct brd_value_string object_string;
extern struct brd_value_string dict_string;
extern struct brd_value_string true_string;
extern struct brd_value_string false_string;

/* the @Object class */
extern struct brd_value object_class;

#endif
